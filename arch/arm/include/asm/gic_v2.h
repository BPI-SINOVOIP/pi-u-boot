#ifndef __ASM_GIC_V2_H
#define __ASM_GIC_V2_H

void gic_v2_init(void __iomem *gicd, void __iomem *gicc, int *irq_num);
void gic_v2_enable_irq(int id);
void gic_v2_disable_irq(int id);
int gic_v2_get_irq_id(void);
void gic_v2_inactive_irq(int irq);
void gic_v2_sgi_trig(int irq);

#endif