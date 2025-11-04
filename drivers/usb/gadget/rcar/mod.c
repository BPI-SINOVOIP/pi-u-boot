/*
 * Renesas USB driver
 *
 * Copyright (C) 2011 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
  *
 * Ported to u-boot
 * Copyright (C) 2016 GlobalLogic
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "common.h"
#include "mod.h"

#define usbhs_priv_to_modinfo(priv) (&priv->mod_info)
#define usbhs_mod_info_call(priv, func, param...)	\
({						\
	struct usbhs_mod_info *info;		\
	info = usbhs_priv_to_modinfo(priv);	\
	!info->func ? 0 :			\
	 info->func(param);			\
})

/*
 *		autonomy
 *
 * these functions are used if platform doesn't have external phy.
 *  -> there is no "notify_hotplug" callback from platform
 *  -> call "notify_hotplug" by itself
 *  -> use own interrupt to connect/disconnect
 *  -> it mean module clock is always ON
 *             ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
static int usbhsm_autonomy_get_vbus(struct platform_device *pdev)
{
	struct usbhs_priv *priv = usbhs_pdev_to_priv(pdev);

	return  VBSTS & usbhs_read(priv, INTSTS0);
}

void usbhsc_notify_hotplug(struct usbhs_priv *priv);

static int usbhsm_autonomy_irq_vbus(struct usbhs_priv *priv,
				    struct usbhs_irq_state *irq_state)
{
	pr_dbg("This is ++usbhsm_autonomy_irq_vbus++\n ");
	usbhsc_notify_hotplug(priv);

	return 0;
}

void usbhs_mod_autonomy_mode(struct usbhs_priv *priv)
{
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);

	pr_dbg("++%s\n", __func__);

	info->irq_vbus		= usbhsm_autonomy_irq_vbus;
	priv->pfunc.get_vbus	= usbhsm_autonomy_get_vbus;

	usbhs_irq_callback_update(priv, NULL);
	pr_dbg("--%s\n", __func__);
}

/*
 *		host / gadget functions
 *
 * renesas_usbhs host/gadget can register itself by below functions.
 * these functions are called when probe
 *
 */
void usbhs_mod_register(struct usbhs_priv *priv, struct usbhs_mod *mod, int id)
{
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);

	info->mod[id]	= mod;
	mod->priv	= priv;
}

struct usbhs_mod *usbhs_mod_get(struct usbhs_priv *priv, int id)
{
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);
	struct usbhs_mod *ret = NULL;

	switch (id) {
	case USBHS_HOST:
	case USBHS_GADGET:
		ret = info->mod[id];
		break;
	}

	return ret;
}

int usbhs_mod_is_host(struct usbhs_priv *priv)
{
	struct usbhs_mod *mod = usbhs_mod_get_current(priv);
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);

	if (!mod)
		return -EINVAL;

	return info->mod[USBHS_HOST] == mod;
}

struct usbhs_mod *usbhs_mod_get_current(struct usbhs_priv *priv)
{
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);

	return info->curt;
}

int usbhs_mod_change(struct usbhs_priv *priv, int id)
{
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);
	struct usbhs_mod *mod = NULL;
	int ret = 0;

	pr_dbg("++%s(%d)\n", __func__, id);

	/* id < 0 mean no current */
	switch (id) {
	case USBHS_HOST:
	case USBHS_GADGET:
		mod = info->mod[id];
		break;
	default:
		ret = -EINVAL;
	}
	info->curt = mod;
	pr_dbg("--%s\n", __func__);
	return ret;
}

static irqreturn_t usbhs_interrupt(int irq, void *data);
int usbhs_mod_probe(struct usbhs_priv *priv)
{
	struct device *dev = usbhs_priv_to_dev(priv);
	int ret;
	pr_dbg("++%s\n", __func__);
	ret = usbhs_mod_gadget_probe(priv);

	/* irq settings */
	ret = devm_request_irq(dev, priv->irq, usbhs_interrupt,
			  priv->irqflags, RCAR3_USBHS_DEVICE, priv);

	pr_dbg("--%s\n", __func__);

	return ret;
}

void usbhs_mod_remove(struct usbhs_priv *priv)
{
	usbhs_mod_gadget_remove(priv);
}

/*
 *		status functions
 */
int usbhs_status_get_device_state(struct usbhs_irq_state *irq_state)
{
	int state = irq_state->intsts0 & DVSQ_MASK;

	switch (state) {
	case POWER_STATE:
	case DEFAULT_STATE:
	case ADDRESS_STATE:
	case CONFIGURATION_STATE:
		return state;
	}

	return -EIO;
}

int usbhs_status_get_ctrl_stage(struct usbhs_irq_state *irq_state)
{
	/*
	 * return value
	 *
	 * IDLE_SETUP_STAGE
	 * READ_DATA_STAGE
	 * READ_STATUS_STAGE
	 * WRITE_DATA_STAGE
	 * WRITE_STATUS_STAGE
	 * NODATA_STATUS_STAGE
	 * SEQUENCE_ERROR
	 */
	return (int)irq_state->intsts0 & CTSQ_MASK;
}

static int usbhs_status_get_each_irq(struct usbhs_priv *priv,
				     struct usbhs_irq_state *state)
{
	struct usbhs_mod *mod = usbhs_mod_get_current(priv);
	u16 intenb1;
	unsigned long flags;

	/********************  spin lock ********************/
	usbhs_lock(priv, flags);
	state->intsts0 = usbhs_read(priv, INTSTS0);

	if (usbhs_mod_is_host(priv)) {
		pr_dbg("WARNING! We are in a host mode!\n");
		state->intsts1 = usbhs_read(priv, INTSTS1);
		intenb1 = usbhs_read(priv, INTENB1);
	} else {
		state->intsts1 = intenb1 = 0;
	}

	/* mask */
	if (mod) {
		state->brdysts = usbhs_read(priv, BRDYSTS);
		state->nrdysts = usbhs_read(priv, NRDYSTS);
		state->bempsts = usbhs_read(priv, BEMPSTS);

//We don't need it here, since driver works in pooling mode;
//		state->bempsts &= mod->irq_bempsts;
//		state->brdysts &= mod->irq_brdysts;
	}
	usbhs_unlock(priv, flags);
	/********************  spin unlock ******************/

	return 0;
}

/*
 *		interrupt
 */
#define INTSTS0_MAGIC 0xF800 /* acknowledge magical interrupt sources */
#define INTSTS1_MAGIC 0xA870 /* acknowledge magical interrupt sources */
static irqreturn_t usbhs_interrupt(int irq, void *data)
{
	struct usbhs_priv *priv = data;
	struct usbhs_irq_state irq_state;

	if (usbhs_status_get_each_irq(priv, &irq_state) < 0)
		return IRQ_NONE;

	/*
	 * clear interrupt
	 *
	 * The hardware is _very_ picky to clear interrupt bit.
	 * Especially INTSTS0_MAGIC, INTSTS1_MAGIC value.
	 *
	 * see
	 *	"Operation"
	 *	 - "Control Transfer (DCP)"
	 *	   - Function :: VALID bit should 0
	 */
	/*
	 * call irq callback functions
	 * see also
	 *	usbhs_irq_setting_update
	 */

	/*It appears that it's more appropriate to clear bits one by one for those
	* interrupts that were handled and avoid writing to BEMPSTS,BRDYSTS and
	* NRDYSTS if IRQ flag was not altered;
	*
	* Such handling doesn't cause USB hang during bulk-out transfer
	*/

	/* INTSTS0 */
	if (irq_state.intsts0 & VBINT) {
		pr_irq("++->VBINT\n");
		usbhs_write(priv, INTSTS0, ~VBINT  & INTSTS1_MAGIC);
		usbhs_mod_info_call(priv, irq_vbus, priv, &irq_state);
	}
	if (irq_state.intsts0 & DVST) {
		pr_irq("++->DVST\n");
		usbhs_write(priv, INTSTS0, ~DVST  & INTSTS1_MAGIC);
		usbhs_mod_call(priv, irq_dev_state, priv, &irq_state);
	}
	if (irq_state.intsts0 & CTRT) {
		pr_irq("++->CTRT\n");
		usbhs_write(priv, INTSTS0, ~CTRT  & INTSTS1_MAGIC);
		usbhs_mod_call(priv, irq_ctrl_stage, priv, &irq_state);
		pr_irq("--->CTRT\n");
	}
	if (irq_state.intsts0 & BEMP) {
		pr_irq("++->BEMP\n");
		usbhs_write(priv, BEMPSTS, ~irq_state.bempsts);
		usbhs_mod_call(priv, irq_empty, priv, &irq_state);
	}
	if (irq_state.intsts0 & BRDY) {
		pr_irq("++->BRDY\n");
		usbhs_write(priv, BRDYSTS, ~irq_state.brdysts);
		usbhs_mod_call(priv, irq_ready, priv, &irq_state);
	}
	if (irq_state.intsts0 & NRDY) {
		pr_irq("++->NRDY\n");
		/*
		* FIXME: Driver has no handler for such kind of interrupt..
		* So just clear the register..
		*/
		usbhs_write(priv, NRDYSTS, ~irq_state.nrdysts);
	}
	if (usbhs_mod_is_host(priv)) {
		/* INTSTS1 */
		pr_irq("WARNING! We are in a host mode!\n");
		usbhs_write(priv, INTSTS1, ~irq_state.intsts1 & INTSTS1_MAGIC);
	}
	return IRQ_HANDLED;
}

void usbhs_irq_callback_update(struct usbhs_priv *priv, struct usbhs_mod *mod)
{
	u16 intenb0 = 0;
	u16 intenb1 = 0;
	struct usbhs_mod_info *info = usbhs_priv_to_modinfo(priv);

	pr_irq("++%s\n", __func__);

	/*
	 * BEMPENB/BRDYENB are picky.
	 * below method is required
	 *
	 *  - clear  INTSTS0
	 *  - update BEMPENB/BRDYENB
	 *  - update INTSTS0
	 */
	usbhs_write(priv, INTENB0, 0);
	if (usbhs_mod_is_host(priv))
		usbhs_write(priv, INTENB1, 0);

	usbhs_write(priv, BEMPENB, 0);
	usbhs_write(priv, BRDYENB, 0);

	/*
	 * see also
	 *	usbhs_interrupt
	 */

	/*
	 * it don't enable DVSE (intenb0) here
	 * but "mod->irq_dev_state" will be called.
	 */
	if (info->irq_vbus)
		intenb0 |= VBSE;

	if (mod) {
		/*
		 * INTSTS0
		 */
		if (mod->irq_ctrl_stage)
			intenb0 |= CTRE;

		if (mod->irq_empty && mod->irq_bempsts) {
			usbhs_write(priv, BEMPENB, mod->irq_bempsts);
			intenb0 |= BEMPE;
		}

		if (mod->irq_ready && mod->irq_brdysts) {
			usbhs_write(priv, BRDYENB, mod->irq_brdysts);
			intenb0 |= BRDYE;
		}

		if (usbhs_mod_is_host(priv)) {
			/*
			 * INTSTS1
			 */
			if (mod->irq_attch)
				intenb1 |= ATTCHE;

			if (mod->irq_dtch)
				intenb1 |= DTCHE;

			if (mod->irq_sign)
				intenb1 |= SIGNE;

			if (mod->irq_sack)
				intenb1 |= SACKE;
		}
	}

	if (intenb0)
		usbhs_write(priv, INTENB0, intenb0);

	if (usbhs_mod_is_host(priv) && intenb1)
		usbhs_write(priv, INTENB1, intenb1);
	pr_irq("--%s\n", __func__);
}

