/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  MbSeqViewController.h
//  MbSeq
//
//  Created by Thorsten Klose on 15.04.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <CLcdView.h>

@interface MbSeqViewController : UIViewController {

IBOutlet CLcdView *cLcdLeft;
IBOutlet CLcdView *cLcdRight;

}

@end

