// texture loading
-(GLuint)loadTexture:(const char*)textureFileName {
    NSBundle *appBundle = [NSBundle mainBundle];
    NSString *appDirPath = [appBundle bundlePath];
    NSString *parentDirPath = [appDirPath stringByDeletinglastPathComponent];
    NSString* textureFileNameWithPath = [NSString stringWithFormat:@"%@/%s", parentDirPath, textureFileName];

    NSImage *nsImage = [[NSImage alloc]initWithContentsOfFile:textureFileNameWithPath];
    if(nsImage == nil) {
        fprintf(gpFile, "Failed to load texture file %s\n", textureFileName);
        return 0;
    }

    CGImageRef cgImage = [nsImage CGImageForProposedRect:nil context:nil hints:nil];
    int imageWidth = (int)CGImageGetWidth(cgImage);
    int imageHeight = (int)CGImageGetHeight(cgImage);

    CGDataProviderRef cgDataProvider = CGImageGetDataProvider(cgImage);
    CFDataRef cfData = CGDataProviderCopyData(cgDataProvider);
    void* imageData = (void*)(CFDataGetBytePtr(cfData));

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // for width not multiple of 4       
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    CFRelease(cfData);
    return texture;
}

// call like this
// GLuint textureStone = [self loadTexture:"Stone.bmp"];

// for tessellation shader setting window title in display function
{
    NSString *msg = [NSString stringWithFormat:@"LRC macOS Window: %d", noOfSegments];
    //[window setTitle:msg];

    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self window]) {
            [[self window] setTitle:msg];
        }
    });
}

// arrow keys in macOS
// case NSUpArrowFunctionKey:
// case NSDownArrowFunctionKey: