#include "IosDisplayInterface.h"
#include "PlayerConfig.h"
#import  "GLView.h"

#pragma mark -

@implementation PlayerGLView

@synthesize m_context;

//相对于屏幕的坐标系 (0, 0) 为屏幕正中
static const GLfloat squareVertices[] =
{
	-1.0f, -1.0f,
	1.0f,  -1.0f,
	-1.0f,  1.0f,
	1.0f,   1.0f
};


//相对于材质的坐标系 (0, 0)左下角
static const GLfloat squareTexCoords[] =
{
	0.0f,  1.0f,
    1.0f,  1.0f,
	0.0f,  0.0f,
	1.0f,  0.0f
};



+ (Class)layerClass 
{
    return [CAEAGLLayer class];
}

/*
- (void)layoutSubviews
{
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        @synchronized(self)
        {
            [EAGLContext setCurrentContext:m_context];
            [self destroyFramebuffer];
            [self createFramebuffers];
        }
        
        glViewport(0, 0, self.bounds.size.width*m_viewScale, self.bounds.size.height*m_viewScale);
        
        NSLog(@"layoutSubviews %f %f",self.bounds.size.width*m_viewScale,self.bounds.size.height*m_viewScale);
    });
}*/

- (id) initWithFrame:(CGRect)frame
{
    if((self = [super initWithFrame:frame]))
	{
	    m_texturWidth = 0;
        m_texturHeight = 0;
        m_viewFramebuffer = 0;
        m_viewRenderbuffer = 0;
        
        m_eaglLayer = (CAEAGLLayer*) self.layer;
        m_eaglLayer.opaque = YES;
        m_eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGB565, kEAGLDrawablePropertyColorFormat,
                                        nil];
        self.contentScaleFactor = [UIScreen mainScreen].scale;
        m_viewScale = [UIScreen mainScreen].scale;
        NSLog(@"initWithFrame m_viewScale:%d",m_viewScale);
        
		m_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		
		if (!m_context)
		{
			NSLog(@"initWithFrame failed~");
			[self release];
			exit(1);
		}
		
		
		if (![EAGLContext setCurrentContext:m_context]) {
			NSLog(@"Failed to set current OpenGL context");
			exit(1);
		}
	
		[self createFramebuffers];		
		[self loadShaders];
		[self createTexture];
				
        glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
        [m_context presentRenderbuffer:GL_RENDERBUFFER];
       
	}
	
	return self;
}

- (void)drawView
{
    if (m_context == nil)
    {
        return;
    }
    
    JPlayer_MediaFrame frame;
    int ret = getYUV420PFrame(VIDEO_DISPLAY_WAITTIME,&frame);
    if(ret != DISPLAY_PLAYER_OK)
    {
		return;
    }
    
     @synchronized(self)
     {
		  if(  (m_texturWidth  == 0) 
		    || (m_texturHeight == 0)  )
		{
			m_texturWidth = frame.m_width;
			m_texturHeight = frame.m_height;
		}
	    
		if ( (m_texturWidth  != frame.m_width ) 
		  || (m_texturHeight != frame.m_height) ) 
		{
		    [self setVideoSize:frame.m_width height:frame.m_height];
		}
	    
	    
		glEnable(GL_TEXTURE_2D);
		[EAGLContext setCurrentContext:m_context];
		if (m_context)
		{         
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebuffer(GL_FRAMEBUFFER, m_viewFramebuffer);
	        
		   // m_viewScale = [UIScreen mainScreen].scale;
		   // m_backingWidth = self.bounds.size.width * m_viewScale;
		   // m_backingHeight = self.bounds.size.height * m_viewScale;
			glViewport(0, 0, m_backingWidth, m_backingHeight);
		   //  NSLog(@"glViewport %d %d %d",m_viewScale,m_backingWidth,m_backingHeight);
		}
	  
	   //glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_Y]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame.m_width, 
			frame.m_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.m_pData[0]);
		//glUniform1i(m_uniforms[UNIFORM_Y], 0);
	   
	    
	   //glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_U]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame.m_width/2, 
			 frame.m_height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
			 frame.m_pData[0] + frame.m_width*frame.m_height);
	   // glUniform1i(m_uniforms[UNIFORM_U], 1);
	    
	   // glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_V]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame.m_width/2,
		  frame.m_height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
		  frame.m_pData[0] + frame.m_width*frame.m_height*5/4);
	   // glUniform1i(m_uniforms[UNIFORM_V], 2);
	    
	  
		// Update attribute values.
		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
		glEnableVertexAttribArray(ATTRIB_VERTEX);
	    
		glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, 0, 0, squareTexCoords);
		glEnableVertexAttribArray(ATTRIB_TEXCOORD);
	   
		if (m_context)
		{	
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	
			glBindRenderbuffer(GL_RENDERBUFFER, m_viewRenderbuffer);
			[m_context presentRenderbuffer:GL_RENDERBUFFER];
		}
     }
     
    releaseYUV420PFrame(&frame);
}

- (void)clearFrame
{
    if ([self window])
    {
        [EAGLContext setCurrentContext:m_context];
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindRenderbuffer(GL_RENDERBUFFER, m_viewRenderbuffer);
        [m_context presentRenderbuffer:GL_RENDERBUFFER];
    }
    
}


- (BOOL)createFramebuffers
{
	// Onscreen framebuffer object
	glGenFramebuffers(1, &m_viewFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER,m_viewFramebuffer);
	
	glGenRenderbuffers(1, &m_viewRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_viewRenderbuffer);
	
	[m_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:m_eaglLayer];
	
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_backingHeight);
	NSLog(@"Backing width: %d, height: %d", m_backingWidth, m_backingHeight);
    
   // m_backingWidth = self.bounds.size.width * m_viewScale;
   /// m_backingHeight = self.bounds.size.height * m_viewScale;
  //  NSLog(@"bounds width: %d, height: %d", m_backingWidth, m_backingHeight);
	
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_viewRenderbuffer);
	
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
	{
		NSLog(@"Failure with framebuffer generation");
		return NO;
	}	
	
	return YES;
}

- (void)destroyFramebuffer 
{
	if (m_viewFramebuffer)
	{
		glDeleteFramebuffers(1, &m_viewFramebuffer);
		m_viewFramebuffer = 0;
	}
	
	if (m_viewRenderbuffer)
	{
		glDeleteRenderbuffers(1, &m_viewRenderbuffer);
		m_viewRenderbuffer = 0;
	}	
}

-(void)createTexture
{
	glGenTextures(NUM_UNIFORMS, m_videoFrameTexture);
	
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_Y]);		//Y Panel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_REPEAT GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
    glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_U]);		//U Panel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
    glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_V]);		//V Panel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
}

-(void)destroryTexture
{
	glDeleteTextures(NUM_UNIFORMS, m_videoFrameTexture);
}



- (void)dealloc 
{
	[self tearDownGL];
	
    if ([EAGLContext currentContext] == m_context)
        [EAGLContext setCurrentContext:nil];
    
    [m_context release];
	
    [super dealloc];
    
      NSLog(@"dealloc\n");
}


- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.m_context];
    
    [self destroryTexture];
    [self destroyFramebuffer];
    
    if (m_program)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}


//#pragma mark -
//#pragma mark OpenGL ES 2.0 setup methods

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    m_program = glCreateProgram();
	
    // Create and compile vertex shader.
	
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Vertext" ofType:@"glsl"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }
	
	// Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Fragment" ofType:@"glsl"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname]) {
        NSLog(@"Failed to compile fragment shader");
        return NO;
    }
	
	
    // Attach vertex shader to program.
    glAttachShader(m_program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(m_program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(m_program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(m_program, ATTRIB_TEXCOORD, "texCoord");
    
	
    // Link program.
    if (![self linkProgram:m_program]) {
        NSLog(@"Failed to link program: %d", m_program);
        
        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (m_program) {
            glDeleteProgram(m_program);
            m_program = 0;
        }
        
        return NO;
    }
	
	glUseProgram(m_program);
	
	m_uniforms[UNIFORM_Y] = glGetUniformLocation(m_program, "SamplerY");
	m_uniforms[UNIFORM_U] = glGetUniformLocation(m_program, "SamplerU");
	m_uniforms[UNIFORM_V] = glGetUniformLocation(m_program, "SamplerV");
	glUniform1i(m_uniforms[UNIFORM_Y], 0);
    glUniform1i(m_uniforms[UNIFORM_U], 1);
    glUniform1i(m_uniforms[UNIFORM_V], 2);
    

	
	// Update attribute values.
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
	glEnableVertexAttribArray(ATTRIB_VERTEX);
    
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, 0, 0, squareTexCoords);
	glEnableVertexAttribArray(ATTRIB_TEXCOORD);
	
    // Release vertex and fragment shaders.
    if (vertShader)
    {
       // glDetachShader(m_program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader)
    {
       // glDetachShader(m_program, fragShader);
        glDeleteShader(fragShader);
    }
	
	
	return YES;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source)
    {
        NSLog(@"Failed to load vertex shader");
        return FALSE;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        glDeleteShader(*shader);
        return FALSE;
    }
    
    return TRUE;
}



- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (void)setVideoSize:(GLuint)width height:(GLuint)height
{
    m_texturWidth = width;
    m_texturHeight = height;
    
    void *pblackData = malloc(width * height * 1.5);
	if(pblackData)
	{
        memset(pblackData, 0x0, width * height * 1.5);
    }
    
      
    
    [EAGLContext setCurrentContext:m_context];
    
    glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_Y]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, 
		GL_LUMINANCE, GL_UNSIGNED_BYTE, pblackData);
		
    glBindTexture(GL_TEXTURE_2D,m_videoFrameTexture[UNIFORM_U]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, 
		GL_LUMINANCE, GL_UNSIGNED_BYTE, pblackData + width * height);
    
    glBindTexture(GL_TEXTURE_2D, m_videoFrameTexture[UNIFORM_V]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, 
		GL_LUMINANCE, GL_UNSIGNED_BYTE, pblackData + width * height * 5 / 4);
		
    free(pblackData);
}


@end
