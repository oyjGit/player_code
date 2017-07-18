#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <GLKit/GLKit.h>

enum
{
    UNIFORM_Y = 0,
    UNIFORM_U,
	UNIFORM_V,
	NUM_UNIFORMS
};

enum
{
    ATTRIB_VERTEX,
	ATTRIB_TEXCOORD,
    NUM_ATTRIBUTES
};



@interface PlayerGLView : UIView
{
    
@private
	
    ///画布大小
    GLint m_backingWidth;
    GLint m_backingHeight;

	GLint m_texturWidth;
	GLint m_texturHeight;
    
    ///opengl 绘图上下文
    EAGLContext *m_context;
    
    /// 渲染缓冲区
    GLuint  m_viewRenderbuffer;

	///帧缓冲区
    GLuint  m_viewFramebuffer;

	///YUV纹理数组
    GLuint  m_videoFrameTexture[NUM_UNIFORMS];
    
    GLsizei  m_viewScale;
    		
@public
	GLint m_uniforms[NUM_UNIFORMS];
    ///着色器句柄
	GLuint m_program;
    CAEAGLLayer *m_eaglLayer;
}

@property (nonatomic, retain) EAGLContext *m_context;

- (void)drawView;
- (id) initWithFrame:(CGRect)frame;
- (void)clearFrame;


- (BOOL)createFramebuffers;
- (void)destroyFramebuffer;

- (void)createTexture;
- (void)destroryTexture;


// OpenGL ES 2.0 setup methods
- (void)tearDownGL;
- (BOOL)loadShaders;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
- (void)setVideoSize:(GLuint)width height:(GLuint)height;



@end



