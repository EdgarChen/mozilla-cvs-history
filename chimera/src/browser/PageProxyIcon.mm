/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
*
* The contents of this file are subject to the Mozilla Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is the Mozilla browser.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation. Portions created by Netscape are
* Copyright (C) 2002 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s):
*   Joe Hewitt <hewitt@netscape.com> (Original Author)
*/

#import "CHPageProxyIcon.h"
#import "BookmarksService.h"
#import "MainController.h"

@implementation CHPageProxyIcon

- (void) resetCursorRects
{
    NSCursor* cursor;
    
    // XXX provide image for drag-hand cursor
    cursor = [NSCursor arrowCursor];
    [self addCursorRect:NSMakeRect(0,0,[self frame].size.width,[self frame].size.height) cursor:cursor];
    [cursor setOnMouseEntered:YES];
}

- (unsigned int)draggingSourceOperationMaskForLocal:(BOOL)flag
{
    return NSDragOperationGeneric;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    // need to implement this or else mouseDragged isn't called
}

- (void) mouseDragged: (NSEvent*) event
{
  NSPasteboard *pboard;
  NSDictionary* data;
  NSArray* dataVals;
  NSArray* dataKeys;
  NSString* url;
  NSString* title;

  nsAutoString hrefStr, titleStr;
  BookmarksService::GetTitleAndHrefForBrowserView([[[[self window] windowController] getBrowserWrapper] getBrowserView],
                                                  titleStr, hrefStr);
  
  url = [NSString stringWithCharacters: hrefStr.get() length: nsCRT::strlen(hrefStr.get())];
  title = [NSString stringWithCharacters: titleStr.get() length: nsCRT::strlen(titleStr.get())];

  dataVals = [NSArray arrayWithObjects: url, title, nil];
  dataKeys = [NSArray arrayWithObjects: @"url", @"title", nil];
  data = [NSDictionary dictionaryWithObjects:dataVals forKeys:dataKeys];

  pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  [pboard declareTypes:[NSArray arrayWithObject:@"MozURLType"] owner:self];
  [pboard setPropertyList:data forType: @"MozURLType"];
  
  [self dragImage: [MainController createImageForDragging:[self image] title:title]
                    at: NSMakePoint(0,0) offset: NSMakeSize(0,0)
                    event: event pasteboard: pboard source: self slideBack: YES];
}

@end
