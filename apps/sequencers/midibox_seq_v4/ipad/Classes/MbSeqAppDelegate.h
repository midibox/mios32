//
//  MbSeqAppDelegate.h
//  MbSeq
//
//  Created by Thorsten Klose on 15.04.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MbSeqViewController;

@interface MbSeqAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    MbSeqViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet MbSeqViewController *viewController;

@end

