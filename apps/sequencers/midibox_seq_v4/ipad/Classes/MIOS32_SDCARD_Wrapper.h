//
//  MIOS32_SDCARD_Wrapper.h
//
//  Created by Thorsten Klose on 06.01.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface MIOS32_SDCARD_Wrapper : NSObject {

}

#ifdef __cplusplus
extern "C" {
#endif

extern void SDCARD_Wrapper_setDir(NSString *_dirName);
extern NSString *SDCARD_Wrapper_getDir(void);

#ifdef __cplusplus
}
#endif

@end
