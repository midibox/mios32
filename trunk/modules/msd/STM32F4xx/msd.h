// $Id$
// Dummy

#ifndef _MSD_H
#define _MSD_H

#ifdef __cplusplus
extern "C" {
#endif

extern s32 MSD_Init(u32 mode);
extern s32 MSD_Periodic_mS(void);

extern s32 MSD_CheckAvailable(void);

extern s32 MSD_LUN_AvailableSet(u8 lun, u8 available);
extern s32 MSD_LUN_AvailableGet(u8 lun);

extern s32 MSD_RdLEDGet(u16 lag_ms);
extern s32 MSD_WrLEDGet(u16 lag_ms);

#ifdef __cplusplus
}
#endif

#endif /* _MSD_H */
