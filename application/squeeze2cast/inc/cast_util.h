/*
 *  Squeeze2upnp - LMS to uPNP gateway
 *
 *  Squeezelite : (c) Adrian Smith 2012-2014, triode1@btinternet.com
 *  Additions & gateway : (c) Philippe 2014, philippe_44@outlook.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CAST_UTIL_H
#define __CAST_UTIL_H

#include "util_common.h"

typedef enum { CAST_PLAY, CAST_PAUSE, CAST_STOP } tCastAction;

struct sq_metadata_s;
struct sMRConfig;

void  	CastInit(log_level level);
void	CastGetStatus(void *Ctx);
void	CastGetMediaStatus(void *Ctx);

void 	CastStop(void *Ctx);
void 	CastClean(void *Ctx);
#define CastPlay(Ctx)	CastSimple(Ctx, "PLAY")
#define CastPause(Ctx)	CastSimple(Ctx, "PAUSE")
void 	CastSimple(void *Ctx, char *Type);
bool	CastLoad(void *Ctx, char *URI, char *ContentType, struct sq_metadata_s *MetaData);
void 	CastSetVolume(void *p, u8_t Volume);

#endif

