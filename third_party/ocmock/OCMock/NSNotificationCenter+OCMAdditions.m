//---------------------------------------------------------------------------------------
//  $Id: NSNotificationCenter+OCMAdditions.m 57 2010-07-19 06:14:27Z erik $
//  Copyright (c) 2009 by Mulle Kybernetik. See License file for details.
//---------------------------------------------------------------------------------------

#import "NSNotificationCenter+OCMAdditions.h"
#import "OCObserverMockObject.h"


@implementation NSNotificationCenter(OCMAdditions)

- (void)addMockObserver:(OCMockObserver *)notificationObserver name:(NSString *)notificationName object:(id)notificationSender
{
	[self addObserver:notificationObserver selector:@selector(handleNotification:) name:notificationName object:notificationSender];
}

@end
