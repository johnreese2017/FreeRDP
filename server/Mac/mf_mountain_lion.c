/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * OS X Server Event Handling
 *
 * Copyright 2012 Corey Clayton <can.of.tuna@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dispatch/dispatch.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreVideo/CoreVideo.h>
#include <IOKit/IOKitLib.h>
#include <IOSurface/IOSurface.h>

#include "mf_mountain_lion.h"

dispatch_semaphore_t region_sem;
dispatch_semaphore_t data_sem;
dispatch_queue_t screen_update_q;
CGDisplayStreamRef stream;

CGDisplayStreamUpdateRef lastUpdate = NULL;

BYTE* localBuf = NULL;

//CVPixelBufferRef pxbuffer = NULL;
//void *baseAddress = NULL;

//CGContextRef bitmapcontext = NULL;

//CGImageRef image = NULL;

BOOL ready = FALSE;

void (^streamHandler)(CGDisplayStreamFrameStatus, uint64_t, IOSurfaceRef, CGDisplayStreamUpdateRef) =  ^(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frameSurface, CGDisplayStreamUpdateRef updateRef)
{
    
    dispatch_semaphore_wait(region_sem, DISPATCH_TIME_FOREVER);
    
    //may need to move this down
    if(ready == TRUE);
    {
        RFX_RECT rect;
        unsigned long offset_beg;
        unsigned long offset_end;

        rect.x = 0;
        rect.y = 0;
        rect.width = 2880;
        rect.height = 1800;
        //mf_mlion_peek_dirty_region(&rect);
        
        //offset_beg = ((rect.width * 4) * rect.y) + rect.x * 4;
        //offset_end =
        
        //lock surface
        IOSurfaceLock(frameSurface, kIOSurfaceLockReadOnly, NULL);
        //get pointer
        void* baseAddress = IOSurfaceGetBaseAddress(frameSurface);
        //copy region
        
        //offset_beg =

        memcpy(localBuf, baseAddress, rect.width * rect.height * 4);
        
        //unlock surface
        IOSurfaceUnlock(frameSurface, kIOSurfaceLockReadOnly, NULL);
        dispatch_semaphore_signal(data_sem);
    }
    
    if (lastUpdate == NULL)
    {
        CFRetain(updateRef);
        lastUpdate = updateRef;
    }
    else
    {
        CGDisplayStreamUpdateRef tmpRef;
        tmpRef = lastUpdate;
        lastUpdate = CGDisplayStreamUpdateCreateMergedUpdate(tmpRef, updateRef);
        CFRelease(tmpRef);
    }
    
    dispatch_semaphore_signal(region_sem);
};

int mf_mlion_screen_updates_init()
{
    printf("mf_mlion_screen_updates_init()\n");
    CGDirectDisplayID display_id;
    
    display_id = CGMainDisplayID();
    
    screen_update_q = dispatch_queue_create("mfreerdp.server.screenUpdate", NULL);
    
    region_sem = dispatch_semaphore_create(1);
    data_sem = dispatch_semaphore_create(1);
 
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display_id);
    
    size_t pixelWidth = CGDisplayModeGetPixelWidth(mode);
    size_t pixelHeight = CGDisplayModeGetPixelHeight(mode);
    
    CGDisplayModeRelease(mode);
    
    localBuf = malloc(pixelWidth * pixelHeight * 4);

    
    stream = CGDisplayStreamCreateWithDispatchQueue(display_id,
                                                    pixelWidth,
                                                    pixelHeight,
                                                    'BGRA',
                                                    NULL,
                                                    screen_update_q,
                                                    streamHandler);
    /*
    
    CFDictionaryRef opts;
    
    long ImageCompatibility;
    long BitmapContextCompatibility;
    
    void * keys[3];
    keys[0] = (void *) kCVPixelBufferCGImageCompatibilityKey;
    keys[1] = (void *) kCVPixelBufferCGBitmapContextCompatibilityKey;
    keys[2] = NULL;
    
    void * values[3];
    values[0] = (void *) &ImageCompatibility;
    values[1] = (void *) &BitmapContextCompatibility;
    values[2] = NULL;
    
    opts = CFDictionaryCreate(kCFAllocatorDefault, (const void **) keys, (const void **) values, 2, NULL, NULL);
    
    if (opts == NULL)
    {
        printf("failed to create dictionary\n");
        //return 1;
    }
    
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, pixelWidth,
                                          pixelHeight,  kCVPixelFormatType_32ARGB, opts,
                                          &pxbuffer);
    
    if (status != kCVReturnSuccess)
    {
        printf("Failed to create pixel buffer! \n");
        //return 1;
    }
    
    CFRelease(opts);
    
    CVPixelBufferLockBaseAddress(pxbuffer, 0);
    baseAddress = CVPixelBufferGetBaseAddress(pxbuffer);
    
    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();

    bitmapcontext = CGBitmapContextCreate(baseAddress,
                                                 pixelWidth,
                                                 pixelHeight, 8, 4*pixelWidth, rgbColorSpace,
                                                 kCGImageAlphaNoneSkipLast);
    
    if (bitmapcontext == NULL) {
        printf("context = null!!!\n\n\n");
    }
    CGColorSpaceRelease(rgbColorSpace);
    */
    
    return 0;
    
}

int mf_mlion_start_getting_screen_updates()
{
    CGError err;
    
    err = CGDisplayStreamStart(stream);
    if(err != kCGErrorSuccess)
    {
        printf("Failed to start displaystream!! err = %d\n", err);
        return 1;
    }
    
    return 0;

}
int mf_mlion_stop_getting_screen_updates()
{
    CGError err;
    
    err = CGDisplayStreamStop(stream);
    if(err != kCGErrorSuccess)
    {
        printf("Failed to stop displaystream!! err = %d\n", err);
        return 1;
    }
    
    return 0;
    
    return 0;
}

int mf_mlion_get_dirty_region(RFX_RECT* invalid)
{
    dispatch_semaphore_wait(region_sem, DISPATCH_TIME_FOREVER);
    
    if (lastUpdate != NULL)
    {
        mf_mlion_peek_dirty_region(invalid);
        
        CFRelease(lastUpdate);
        
        lastUpdate = NULL;

    }

    
    dispatch_semaphore_signal(region_sem);
    
    return 0;
}

int mf_mlion_peek_dirty_region(RFX_RECT* invalid)
{
    size_t num_rects;
    CGRect dirtyRegion;
    
    const CGRect * rects = CGDisplayStreamUpdateGetRects(lastUpdate, kCGDisplayStreamUpdateDirtyRects, &num_rects);
    
    printf("\trectangles: %zd\n", num_rects);
    
    if (num_rects == 0) {
        //dispatch_semaphore_signal(region_sem);
        return 0;
    }
    
    dirtyRegion = *rects;
    for (size_t i = 0; i < num_rects; i++)
    {
        dirtyRegion = CGRectUnion(dirtyRegion, *(rects+i));
    }
    
    invalid->x = dirtyRegion.origin.x;
    invalid->y = dirtyRegion.origin.y;
    invalid->height = dirtyRegion.size.height;
    invalid->width = dirtyRegion.size.width;
    
    return 0;
}

int mf_mlion_clear_dirty_region()
{
   /* dispatch_semaphore_wait(region_sem, DISPATCH_TIME_FOREVER);

    clean = TRUE;
    dirtyRegion.size.width = 0;
    dirtyRegion.size.height = 0;
    
    dispatch_semaphore_signal(region_sem);
    */
    return 0;
}

int mf_mlion_get_pixelData(long x, long y, long width, long height, BYTE** pxData)
{
    printf("waiting for region semaphore...\n");
    dispatch_semaphore_wait(region_sem, DISPATCH_TIME_FOREVER);
    ready = TRUE;
    printf("waiting for data semaphore...\n");
    dispatch_semaphore_wait(data_sem, DISPATCH_TIME_FOREVER);
    dispatch_semaphore_signal(region_sem);
    
    //this second wait allows us to block until data is copied... more on this later
    dispatch_semaphore_wait(data_sem, DISPATCH_TIME_FOREVER);
    printf("got it\n");
    *pxData = localBuf;
    dispatch_semaphore_signal(data_sem);
    
    /*
    if (image != NULL) {
        CGImageRelease(image);
    }
    image = CGDisplayCreateImageForRect(
                                                   kCGDirectMainDisplay,
                                                   CGRectMake(x, y, width, height) );
    
    CGContextDrawImage(
                       bitmapcontext,
                       CGRectMake(0, 1800 - height, width, height),
                       image);
    
    *pxData = baseAddress;
     
     */
    
    return 0;
}

