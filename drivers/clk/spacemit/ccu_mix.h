// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, spacemit Corporation.
 *
 */

#ifndef _CCU_MIX_H_
#define _CCU_MIX_H_

#include "ccu-k1x.h"

#define CCU_CLK_MIX  "ccu_clk_mix"
#define SPACEMIT_CLK_GATE_NEED_DELAY BIT(0)

struct ccu_gate_config {
	u32		gate_mask;
	u32		val_enable;
	u32		val_disable;
	u32		flags;
};

struct ccu_factor_config {
	u32		div;
	u32		mul;
};

struct ccu_mux_config {
	u8		shift;
	u8		width;
	const u8	*table;
	u32		flags;
};

struct ccu_div_config {
	u8			shift;
	u8			width;
	u32			max;
	u32			offset;
	u32			flags;
	struct clk_div_table	*table;
};

struct ccu_mix {
	struct ccu_gate_config  *gate;
	struct ccu_factor_config  *factor;
	struct ccu_div_config	*div;
	struct ccu_mux_config	*mux;
	struct ccu_common	common;
};

#define CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, _flags)		\
	(&(struct ccu_gate_config) {				\
		.gate_mask   = _gate_mask,			\
		.val_enable  = _val_enable,			\
		.val_disable = _val_disable,	\
		.flags	= _flags,				\
	})

#define CCU_FACTOR_INIT(_div, _mul)		\
	(&(struct ccu_factor_config) {				\
		.div = _div,			\
		.mul = _mul,			\
	})


#define CCU_MUX_INIT(_shift, _width, _table, _flags)		\
	(&(struct ccu_mux_config) {				\
		.shift	= _shift,			\
		.width	= _width,			\
		.table	= _table,			\
		.flags	= _flags,				\
	})

#define CCU_DIV_INIT(_shift, _width, _table, _flags)		\
	(&(struct ccu_div_config) {				\
		.shift	= _shift,					\
		.width	= _width,					\
		.flags	= _flags,					\
		.table	= _table,					\
	})

#define SPACEMIT_CCU_GATE(_struct, _name, _parent, _base_type, _reg,	\
				      _gate_mask, _val_enable, _val_disable, _flags) \
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	 \
		.common	= {						\
			.reg_ctrl		= _reg,				\
			.base_type 		= _base_type,       \
			.name			= _name,	\
			.parent_name	= _parent,	\
			.num_parents	= 1, \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}
#define SPACEMIT_CCU_GATE_NO_PARENT(_struct, _name, _parent, _base_type, _reg,	\
						  _gate_mask, _val_enable, _val_disable, _flags) \
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	 \
		.common = { 					\
			.reg_ctrl		= _reg, 			\
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_name	= SPACEMIT_CLK_NO_PARENT,	\
			.num_parents	= 0, \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}

#define SPACEMIT_CCU_FACTOR(_struct, _name, _parent,	\
						  _div, _mul) \
	struct ccu_mix _struct = {					\
		.factor	= CCU_FACTOR_INIT(_div, _mul),	 \
		.common = { 					\
			.name			= _name,	\
			.parent_name	= _parent,	\
			.num_parents	= 1, \
			.driver_name	= CCU_CLK_MIX, \
		}							\
	}

#define SPACEMIT_CCU_MUX(_struct, _name, _parents, _base_type, _reg,	\
						  _shift, _width, _flags) \
	struct ccu_mix _struct = {					\
		.mux	= CCU_MUX_INIT(_shift, _width, NULL, 0),	 \
		.common = { 					\
			.reg_ctrl		= _reg, 			\
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents,	\
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}

#define SPACEMIT_CCU_DIV(_struct, _name, _parent, _base_type, _reg,	\
							  _shift, _width, _flags) \
	struct ccu_mix _struct = {					\
		.div	= CCU_DIV_INIT(_shift, _width, NULL, 0),	 \
		.common = { 					\
			.reg_ctrl		= _reg, 			\
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_name	= _parent,	\
			.num_parents	= 1, \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}

#define SPACEMIT_CCU_GATE_FACTOR(_struct, _name, _parent, _base_type, _reg,	\
						  _gate_mask, _val_enable, _val_disable,  \
						  _div, _mul, _flags) \
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	 \
		.factor	= CCU_FACTOR_INIT(_div, _mul),	 \
		.common = { 					\
			.reg_ctrl		= _reg, 			\
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_name	= _parent,	\
			.num_parents	= 1, \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}


#define SPACEMIT_CCU_MUX_GATE(_struct, _name, _parents, _base_type, _reg,	\
							  _shift, _width, _gate_mask, _val_enable, _val_disable, _flags) \
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	 \
		.mux	= CCU_MUX_INIT(_shift, _width, NULL, 0),	 \
		.common = { 					\
			.reg_ctrl		= _reg, 			\
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents,	\
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}

#define SPACEMIT_CCU_DIV_GATE(_struct, _name, _parent, _base_type, _reg,	\
								 _shift, _width, _gate_mask, _val_enable, _val_disable, _flags) \
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	 \
		.div	= CCU_DIV_INIT(_shift, _width, NULL, 0),	 \
		.common = { 					\
			.reg_ctrl		= _reg, 			\
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_name	= _parent,	\
			.num_parents	= 1, \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		}							\
	}


#define SPACEMIT_CCU_DIV_MUX_GATE(_struct, _name, _parents,		\
					_base_type, _reg_ctrl,				\
					_mshift, _mwidth,		\
					_muxshift, _muxwidth,		\
					_gate_mask, _val_enable, _val_disable, _flags)			\
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),				\
		.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),		\
		.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0), \
		.common	= {						\
			.reg_ctrl		= _reg_ctrl,				\
			.base_type 		= _base_type,       \
			.name			= _name,	\
			.parent_names	= _parents,	\
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}

#define SPACEMIT_CCU_DIV2_FC_MUX_GATE(_struct, _name, _parents, _base_type, _reg_ctrl, _reg_sel,	\
						  _mshift, _mwidth, _fc, _muxshift, _muxwidth, _gate_mask, _val_enable, _val_disable, 		\
						  _flags)					\
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	\
		.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),		\
		.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0), \
		.common = { 					\
		    .reg_type = CLK_DIV_TYPE_2REG_FC_V4,   \
			.reg_ctrl		= _reg_ctrl,			\
			.reg_sel		= _reg_sel, 	 \
			.fc 			= _fc,		   \
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents,	\
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}


#define SPACEMIT_CCU_DIV_FC_MUX_GATE(_struct, _name, _parents, _base_type, _reg_ctrl,	\
						  _mshift, _mwidth, _fc, _muxshift, _muxwidth, _gate_mask, _val_enable, _val_disable, 		\
						  _flags)					\
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	\
		.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),		\
		.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0), \
		.common = { 					\
		    .reg_type = CLK_DIV_TYPE_1REG_FC_V2,    \
			.reg_ctrl		= _reg_ctrl,			\
			.fc 			= _fc,		   \
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents,	\
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}

#define SPACEMIT_CCU_DIV_MFC_MUX_GATE(_struct, _name, _parents, _base_type, _reg_ctrl,	\
						  _mshift, _mwidth, _fc, _muxshift, _muxwidth, _gate_mask, _val_enable, _val_disable,		\
						  _flags)					\
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	\
		.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),		\
		.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0), \
		.common = { 					\
			.reg_type = CLK_DIV_TYPE_1REG_FC_MUX_V6,	\
			.reg_ctrl		= _reg_ctrl,			\
			.fc 			= _fc,		   \
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents, \
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}

#define SPACEMIT_CCU_DIV_FC_WITH_GATE(_struct, _name, _parent, _base_type, _reg_ctrl,		\
					  _mshift, _mwidth, _fc, _gate_mask, _val_enable, _val_disable,			\
					  _flags)					\
	struct ccu_mix _struct = {					\
		.gate	= CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, 0),	\
		.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),		\
		.common = { 					\
			.reg_type = CLK_DIV_TYPE_1REG_FC_V2,    \
			.reg_ctrl		= _reg_ctrl, 			\
			.fc 			= _fc,         \
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_name	= _parent,	\
			.num_parents	= 1, \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}

#define SPACEMIT_CCU_DIV_FC_MUX(_struct, _name, _parents, _base_type, _reg_ctrl,		\
					  _mshift, _mwidth, _fc, _muxshift, _muxwidth, _flags)					\
	struct ccu_mix _struct = {					\
		.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),		\
		.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0), \
		.common = { 					\
			.reg_type = CLK_DIV_TYPE_1REG_FC_V2,    \
			.reg_ctrl		= _reg_ctrl, 			\
			.fc 			= _fc,         \
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents,	\
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}

#define SPACEMIT_CCU_MUX_FC(_struct, _name, _parents, _base_type, _reg_ctrl,		\
			_fc, _muxshift, _muxwidth, _flags)					\
	struct ccu_mix _struct = {					\
		.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0), \
		.common = { 					\
			.reg_type = CLK_DIV_TYPE_1REG_FC_V2,	\
			.reg_ctrl		= _reg_ctrl,			\
			.fc 			= _fc,		   \
			.base_type		= _base_type,		\
			.name			= _name,	\
			.parent_names	= _parents, \
			.num_parents	= ARRAY_SIZE(_parents), \
			.driver_name	= CCU_CLK_MIX, \
			.flags			= _flags, \
		},							\
	}

static inline struct ccu_mix *clk_to_ccu_mix(struct clk *clk)
{
	struct ccu_common *common = clk_to_ccu_common(clk);

	return container_of(common, struct ccu_mix, common);
}

unsigned int ccu_mix_get_parent(struct clk *clk);

#endif /* _CCU_DIV_H_ */
