#include <common.h>
#include <mmc.h>
#include "miniloader.h"
#include "sm.h"

extern struct mmc *init_mmc_device(int dev, bool force_init);

unsigned int load_miniloader_image(int devid, int boot_hwpart, unsigned int *paddr)
{
	struct bootflag *tmp = (struct bootflag *)SM_BOOTMODE_ADDR;
	printf("magic[%x], mm_addr[%lx], mf_addr[%x] mf_size[%x]\n",
		tmp->magic, tmp->mm_addr, tmp->mf_addr, tmp->mf_size);

	struct mmc *mmc;
	mmc = init_mmc_device(devid, false);

	struct blk_desc *dev;
	dev = mmc_get_blk_desc(mmc);

	int ret = blk_dselect_hwpart(dev, boot_hwpart);
	printf("switch to partitions #%d, %s\n",
		   boot_hwpart, (!ret) ? "OK" : "ERROR");
	if (ret)
		return -EIO;

	ret = blk_dread(dev, tmp->mf_addr/512, tmp->mf_size/512,
		(unsigned char *)tmp->mm_addr);
	if (ret != tmp->mf_size/512)
		return -EIO;

	*paddr = tmp->mm_addr;
	flush_dcache_all();

	return tmp->mf_size;
}

void jump_miniloader(void)
{
	void (*mini_entry)(void);
	struct bootflag *tmp = (struct bootflag *)SM_BOOTMODE_ADDR;

	dcache_disable();
	icache_disable();

	mini_entry = (void (*)(void))((unsigned long)(tmp->mm_addr));
	mini_entry();
}
