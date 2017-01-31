/*
 * MIDIbox Quad Genesis: File browser
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "filebrowser.h"

#include <file.h>
#include <string.h>
#include <vgm.h>

#include "frontpanel.h"
#include "interface.h"
#include "nameeditor.h"

static void (*callback_f)(char* filename);
static char* curpath;
static char* curname;
static char* cursubdir;
static char extn[4];
static u8 waserror;
static u8 saving;
static u8 cursor;
static u8 innameeditor;

static void DrawMenu(){
    //-----=====-----=====-----=====-----=====
    //In /vgm/SEGA/
    //Up   >Dir 12345678< >File 12345678.vgm<
    //-----=====-----=====-----=====-----=====
    if(waserror || !FILE_VolumeAvailable()){
        MIOS32_LCD_Clear();
        MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintString("SD card not mounted, or directory error!");
        DBG("ERROR curpath = %s", curpath);
        DBG("      curname = %s", curname);
        DBG("    cursubdir = %s", cursubdir);
        return;
    }
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintFormattedString("In %s%c", curpath, curpath[1] == 0 ? ' ' : '/');
    MIOS32_LCD_CursorSet(0,1);
    MIOS32_LCD_PrintFormattedString("Up    Dir %s", cursubdir[0] == 0 ? (saving ? "[new]" : "[no dir]") : cursubdir);
    MIOS32_LCD_CursorSet(21,1);
    MIOS32_LCD_PrintFormattedString("File %s", curname[0] == 0 ? (saving ? "[new]" : "[none]") : curname);
    MIOS32_LCD_CursorSet(cursor ? 20 : 5, 1);
    MIOS32_LCD_PrintChar(0x7E);
    MIOS32_LCD_CursorSet(cursor ? 38 : 18, 1);
    MIOS32_LCD_PrintChar(0x7F);
}

static inline void GotFile(){
    sprintf((char*)(curpath[1] == 0 ? curpath : curpath + strlen(curpath)), "/%s", curname);
    subscreen = 0;
    callback_f(curpath);
}
static inline void GotDir(){
    sprintf((char*)(curpath[1] == 0 ? curpath : curpath + strlen(curpath)), "/%s", cursubdir);
    if(!FILE_DirExists(curpath)){
        if(FILE_MakeDir(curpath) < 0) waserror = 1;
    }
    cursubdir[0] = 0;
    s32 ret = FILE_FindNextFile(curpath, NULL, extn, curname);
    if(ret < 0) waserror = 1;
    ret = FILE_FindNextDir(curpath, NULL, cursubdir);
    if(ret < 0) waserror = 1;
    cursor = (cursubdir[0] == 0); //If there's no subdirectories, point cursor to files
    DrawMenu();
}

static void NameEditorFinished(){
    innameeditor = 0;
    if(cursor){
        //Was editing file name
        //Add extension
        sprintf(curname + strlen(curname), ".%s", extn);
        //Use file
        GotFile();
        return;
    }else{
        //Was editing directory name
        //Create and use directory
        GotDir();
        return;
    }
}

void Filebrowser_Init(){
    curpath = vgmh2_malloc(256);
    curname = vgmh2_malloc(13);
    cursubdir = vgmh2_malloc(9);
    curpath[0] = 0;
    curname[0] = 0;
    cursubdir[0] = 0;
}
void Filebrowser_Start(const char* initpath, const char* extension, u8 save, void (*callback)(char* filename)){
    subscreen = SUBSCREEN_FILEBROWSER;
    callback_f = callback;
    saving = save;
    cursor = 0;
    waserror = 0;
    innameeditor = 0;
    s32 ret = 0;
    MUTEX_SDCARD_TAKE;
    if(!FILE_VolumeAvailable()){
        ret = -69;
        goto Error;
    }
    /*
    extn[0] = 0;
    DBG("FILE_FileExists(\"\") == %d", FILE_FileExists(extn));
    DBG("FILE_DirExists(\"\") == %d", FILE_DirExists(extn));
    */
    //
    strcpy(extn, extension);
    if(initpath == NULL){
        //Was the last thing in curpath a directory (dialog was quit from)?
        if(FILE_DirExists(curpath) == 1 && curpath[0] != 0){
            //Leave everything as is
            goto Done;
        }
        //Was the last thing in curpath a file (we selected a file)?
        if(FILE_FileExists(curpath) == 1){
            //Chop off the filename and put that into curname
            u8 i = strlen(curpath)-1;
            while(curpath[i] != '/') --i;
            if(i != 0) curpath[i] = 0; //Get rid of slash unless it's root
            ++i;
            u8 j = 0;
            while(curpath[i] != 0){
                curname[j] = curpath[i];
                curpath[i] = 0;
                ++j;
                ++i;
            }
            curname[j] = 0;
            cursor = 1;
            goto Done;
        }
        //Otherwise, just start over from root
        curpath[0] = '/';
        curpath[1] = 0;
    }else{
        strcpy(curpath, initpath);
    }
    //Initialize new path, get first file and directory
    ret = FILE_FindNextFile(curpath, NULL, extn, curname);
    if(ret == FILE_ERR_NO_DIR){
        ret = FILE_MakeDir(curpath);
        if(ret < 0) goto Error;
        ret = FILE_FindNextFile(curpath, NULL, extn, curname);
    }
    if(ret < 0) goto Error;
    ret = FILE_FindNextDir(curpath, NULL, cursubdir);
    if(ret < 0) goto Error;
    cursor = (cursubdir[0] == 0); //If there's no subdirectories, point cursor to files
    goto Done;
Error:
    waserror = 1;
    DBG("Filebrowser_Start error %d", ret);
Done:
    MUTEX_SDCARD_GIVE;
    DrawMenu();
}

void Filebrowser_BtnSoftkey(u8 softkey, u8 state){
    if(innameeditor){
        NameEditor_BtnSoftkey(softkey, state);
        return;
    }
    if(!state) return;
    if(waserror || !FILE_VolumeAvailable()){
        waserror = 1;
        DrawMenu();
        return;
    }
    if(softkey == 0){
        if(curpath[1] == 0) return; //We're at root, don't go up
        //Chop off the filename and put that into cursubdir
        u8 i = strlen(curpath)-1;
        while(curpath[i] != '/') --i;
        if(i != 0) curpath[i] = 0; //Get rid of slash unless it's root
        ++i;
        u8 j = 0;
        while(curpath[i] != 0){
            cursubdir[j] = curpath[i];
            curpath[i] = 0;
            ++j;
            ++i;
        }
        cursubdir[j] = 0;
        s32 ret = FILE_FindNextFile(curpath, NULL, extn, curname);
        if(ret < 0) waserror = 1;
        cursor = 0;
        DrawMenu();
    }else if(softkey <= 3){
        cursor = 0;
        DrawMenu();
    }else{
        cursor = 1;
        DrawMenu();
    }
}
void Filebrowser_BtnSystem(u8 button, u8 state){
    if(innameeditor){
        NameEditor_BtnSystem(button, state);
        return;
    }
    if(!state) return;
    if(button == FP_B_MENU){
        subscreen = 0;
        callback_f(NULL);
        return;
    }
    if(waserror || !FILE_VolumeAvailable()){
        waserror = 1;
        DrawMenu();
        return;
    }
    if(button == FP_B_ENTER){
        if(cursor){
            //Press Enter on file
            if(curname[0] == 0){
                if(saving){
                    //Prompt for new file
                    innameeditor = 1;
                    curname[0] = 'A';
                    curname[1] = 0;
                    NameEditor_Start(curname, 8, "New file", &NameEditorFinished);
                }else{
                    return;
                }
            }else{
                //Return file name
                GotFile();
                return;
            }
        }else{
            //Press Enter on directory
            if(cursubdir[0] == 0){
                if(saving){
                    //Prompt for new subdirectory
                    innameeditor = 1;
                    cursubdir[0] = 'A';
                    cursubdir[1] = 0;
                    NameEditor_Start(cursubdir, 8, "New dir", &NameEditorFinished);
                }else{
                    return;
                }
            }else{
                //Enter subdirectory
                GotDir();
                return;
            }
        }
    }
}
void Filebrowser_EncDatawheel(s32 incrementer){
    if(innameeditor){
        NameEditor_EncDatawheel(incrementer);
        return;
    }
    if(waserror || !FILE_VolumeAvailable()){
        waserror = 1;
        DrawMenu();
        return;
    }
    if(cursor){
        if(incrementer > 0){
            MUTEX_SDCARD_TAKE;
            char* newname = vgmh2_malloc(13);
            if(FILE_FindNextFile(curpath, curname[0] == 0 ? NULL : curname, extn, newname) < 0) waserror = 1;
            strcpy(curname, newname);
            vgmh2_free(newname);
            MUTEX_SDCARD_GIVE;
            DrawMenu();
            return;
        }else if(incrementer < 0){
            MUTEX_SDCARD_TAKE;
            char* newname = vgmh2_malloc(13);
            if(FILE_FindPreviousFile(curpath, curname[0] == 0 ? NULL : curname, extn, newname) < 0) waserror = 1;
            strcpy(curname, newname);
            vgmh2_free(newname);
            MUTEX_SDCARD_GIVE;
            DrawMenu();
            return;
        }
    }else{
        if(incrementer > 0){
            MUTEX_SDCARD_TAKE;
            char* newdir = vgmh2_malloc(9);
            if(FILE_FindNextDir(curpath, cursubdir[0] == 0 ? NULL : cursubdir, newdir) < 0) waserror = 1;
            strcpy(cursubdir, newdir);
            vgmh2_free(newdir);
            MUTEX_SDCARD_GIVE;
            DrawMenu();
            return;
        }else if(incrementer < 0){
            MUTEX_SDCARD_TAKE;
            char* newdir = vgmh2_malloc(9);
            if(FILE_FindPreviousDir(curpath, cursubdir[0] == 0 ? NULL : cursubdir, newdir) < 0) waserror = 1;
            strcpy(cursubdir, newdir);
            vgmh2_free(newdir);
            MUTEX_SDCARD_GIVE;
            DrawMenu();
            return;
        }
    }
}
