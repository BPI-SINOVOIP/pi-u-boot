// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 */

#include <common.h>
#include <linux/compiler.h>
#include <efi_loader.h>


DECLARE_GLOBAL_DATA_PTR;
#ifdef CONFIG_USE_IRQ_SYNA

#include <malloc.h>
#include <asm/gic_v2.h>

struct irq_action {
	interrupt_handler_t *handler;
	void *arg;
	unsigned int count;
};

static struct irq_action *irq_handlers;
static int irq_num = 0;
static int spurious_irq_cnt;
void sgi_isr(void *arg);

int interrupt_init(void)
{
	gic_v2_init((void *)GICD_BASE, (void *)GICC_BASE, &irq_num);
	spurious_irq_cnt = 0;
	irq_handlers = (struct irq_action *)malloc(irq_num * sizeof(struct irq_action));
	memset(irq_handlers, 0, irq_num * sizeof(struct irq_action));

#ifdef CONFIG_CMD_IRQ_SYNA
	int i;
	for (i = 0; i < 16; i++)
	{
		gic_v2_enable_irq(i);
		irq_handlers[i].handler = sgi_isr;
		irq_handlers[i].arg = (void *)(ulong)i;
		irq_handlers[i].count = 0;
	}
#endif

	return 0;
}

void enable_interrupts(void)
{
	__asm__ __volatile__ ("msr	daifclr, #2" : : : "memory");
	return;
}

int disable_interrupts(void)
{
	__asm__ __volatile__ ("msr	daifset, #2" : : : "memory");
	return 0;
}

void irq_install_handler(int irq, interrupt_handler_t *handler, void *arg)
{
	disable_interrupts();

	gic_v2_enable_irq(irq);

	irq_handlers[irq].handler = handler;
	irq_handlers[irq].arg = arg;
	irq_handlers[irq].count = 0;

	enable_interrupts();

	return;
}

void irq_free_handler(int irq)
{
	disable_interrupts();

	gic_v2_disable_irq(irq);

	irq_handlers[irq].handler = NULL;
	irq_handlers[irq].arg = NULL;
	irq_handlers[irq].count = 0;

	enable_interrupts();

	return;
}

#ifdef CONFIG_CMD_IRQ_SYNA
int do_irqinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int irq;

	printf("Spurious IRQ: %u\n", spurious_irq_cnt);

	printf("Interrupt-Information:\n");
	printf("Nr  Routine   Arg       Count\n");

	for (irq = 0; irq < irq_num; irq++) {
		if (irq_handlers[irq].handler != NULL) {
			printf("%02d  %08lx  %08lx  %d\n",
					irq,
					(ulong)irq_handlers[irq].handler,
					(ulong)irq_handlers[irq].arg,
					irq_handlers[irq].count);
		}
	}

	return 0;
}

__weak void sgi_isr(void *arg)
{
	printf("SGI Received, irq index = %ld\n", (ulong)arg);
}

static int do_sgi(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int irq;
	if (argc <= 1)
		irq = 0;
	else
		irq = simple_strtoul(argv[1], NULL, 10);
	gic_v2_sgi_trig(irq & 0xf);
	return 0;
}

U_BOOT_CMD(
     sgi,    2,    0,    do_sgi,
     NULL,
     NULL
);

#endif
#else
int interrupt_init(void)
{
	return 0;
}

void enable_interrupts(void)
{
	return;
}

int disable_interrupts(void)
{
	return 0;
}
#endif
static void show_efi_loaded_images(struct pt_regs *regs)
{
	efi_print_image_infos((void *)regs->elr);
}

void show_regs(struct pt_regs *regs)
{
	int i;

	if (gd->flags & GD_FLG_RELOC)
		printf("elr: %016lx lr : %016lx (reloc)\n",
		       regs->elr - gd->reloc_off,
		       regs->regs[30] - gd->reloc_off);
	printf("elr: %016lx lr : %016lx\n", regs->elr, regs->regs[30]);

	for (i = 0; i < 29; i += 2)
		printf("x%-2d: %016lx x%-2d: %016lx\n",
		       i, regs->regs[i], i+1, regs->regs[i+1]);
	printf("\n");
}

/*
 * do_bad_sync handles the impossible case in the Synchronous Abort vector.
 */
void do_bad_sync(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("Bad mode in \"Synchronous Abort\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_bad_irq handles the impossible case in the Irq vector.
 */
void do_bad_irq(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("Bad mode in \"Irq\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_bad_fiq handles the impossible case in the Fiq vector.
 */
void do_bad_fiq(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("Bad mode in \"Fiq\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_bad_error handles the impossible case in the Error vector.
 */
void do_bad_error(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("Bad mode in \"Error\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_sync handles the Synchronous Abort exception.
 */
void do_sync(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("\"Synchronous Abort\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_irq handles the Irq exception.
 */
#ifdef CONFIG_USE_IRQ_SYNA
void do_irq(struct pt_regs *pt_regs, unsigned int esr)
{
	int irq = gic_v2_get_irq_id();
	if (irq == -EINVAL) {
		spurious_irq_cnt++;
		return;
	} else if (irq < 0) {
		efi_restore_gd();
		printf("\"Irq\" handler, esr 0x%08x\n", esr);
		show_regs(pt_regs);
		panic("Resetting CPU ...\n");
	}
	if (irq_handlers[irq].handler) {
		irq_handlers[irq].handler(irq_handlers[irq].arg);
		irq_handlers[irq].count++;
	}
	gic_v2_inactive_irq(irq);
	return;
}
#else
void do_irq(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("\"Irq\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}
#endif

/*
 * do_fiq handles the Fiq exception.
 */
void do_fiq(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("\"Fiq\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_error handles the Error exception.
 * Errors are more likely to be processor specific,
 * it is defined with weak attribute and can be redefined
 * in processor specific code.
 */
void __weak do_error(struct pt_regs *pt_regs, unsigned int esr)
{
	efi_restore_gd();
	printf("\"Error\" handler, esr 0x%08x\n", esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}
