#ifndef __KERNEL_PRINTK__
#define __KERNEL_PRINTK__

#include <log.h>
#include <stdio.h>
#include <linux/compiler.h>

#define KERN_EMERG
#define KERN_ALERT
#define KERN_CRIT
#define KERN_ERR
#define KERN_WARNING
#define KERN_NOTICE
#define KERN_INFO
#define KERN_DEBUG
#define KERN_CONT

#if defined (CONFIG_SPL_BUILD)
#define PRINTK_LOGLEVEL CONFIG_SPL_LOGLEVEL
#else
#define PRINTK_LOGLEVEL CONFIG_LOGLEVEL
#endif

#define printk(fmt, ...) \
	printf(fmt, ##__VA_ARGS__)

/*
 * Dummy printk for disabled debugging statements to use whilst maintaining
 * gcc's format checking.
 */
#define no_printk(fmt, ...)				\
({							\
	if (0)						\
		printk(fmt, ##__VA_ARGS__);		\
	0;						\
})

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_emerg(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 0 ? log_emerg(fmt, ##__VA_ARGS__) : 0;	\
})
#define pr_alert(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 1 ? log_alert(fmt, ##__VA_ARGS__) : 0;	\
})
#define pr_crit(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 2 ? log_crit(fmt, ##__VA_ARGS__) : 0;		\
})
#define pr_err(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 3 ? log_err(fmt, ##__VA_ARGS__) : 0;		\
})
#define pr_warn(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 4 ? log_warning(fmt, ##__VA_ARGS__) : 0;	\
})
#define pr_notice(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 5 ? log_notice(fmt, ##__VA_ARGS__) : 0;	\
})
#define pr_info(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 6 ? log_info(fmt, ##__VA_ARGS__) : 0;		\
})
#define pr_debug(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 7 ? log_debug(fmt, ##__VA_ARGS__) : 0;	\
})
#define pr_devel(fmt, ...)						\
({									\
	PRINTK_LOGLEVEL > 7 ? log_debug(fmt, ##__VA_ARGS__) : 0;	\
})

#ifdef CONFIG_LOG
#define pr_cont(fmt, ...)						\
({									\
	gd->logl_prev < CONFIG_LOGLEVEL ?				\
		log_cont(fmt, ##__VA_ARGS__) : 0;			\
})
#else
#define pr_cont(fmt, ...)						\
	printk(fmt, ##__VA_ARGS__)
#endif

#define printk_once(fmt, ...) \
	printk(fmt, ##__VA_ARGS__)

#endif
