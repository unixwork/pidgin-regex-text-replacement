/*
 * pidgin-regex-text-replacement
 *
 * Copyright (C) 2025 Olaf Wintermann
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RTR_UI_H
#define RTR_UI_H

#include <gtkplugin.h>
#include "regex-text-replacement.h"



GtkWidget *get_config_frame(PurplePlugin *plugin);

#endif /* RTR_UI_H */

