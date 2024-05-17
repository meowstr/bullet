#include "hardware.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "logging.hpp"

static struct {
    GLFWwindow * window = nullptr;
    int width = 800;
    int height = 800;

    int pending_event_list[ 16 ];
    int pending_event_count = 0;

} intern;

static void
handle_mouse_button( GLFWwindow * window, int button, int action, int mods )
{
    if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ) {
        if ( intern.pending_event_count >= 16 ) {
            ERROR_LOG( "input overflow" );
        }
        intern.pending_event_list[ intern.pending_event_count ] = EVENT_TOUCH;
        intern.pending_event_count++;
    }
}

int hardware_init()
{
    if ( !glfwInit() ) {
        ERROR_LOG( "failed to initialize glfw" );
        return 1;
    }

    // glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_ES_API );
    // glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
    // glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
    // glfwWindowHint( GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API );

    glfwWindowHintString( GLFW_X11_CLASS_NAME, "floater" );
    glfwWindowHintString( GLFW_X11_INSTANCE_NAME, "floater" );
    glfwWindowHint( GLFW_RESIZABLE, 0 );

    intern.window = glfwCreateWindow(
        intern.width,
        intern.height,
        "STARIX",
        nullptr,
        nullptr
    );

    if ( !intern.window ) {
        ERROR_LOG( "failed to create window" );
        return 1;
    }

    glfwMakeContextCurrent( intern.window );

    if ( !gladLoadGLES2Loader( (GLADloadproc) glfwGetProcAddress ) ) {
        ERROR_LOG( "failed to load OpenGL" );
        return 1;
    }

    const char * gl_version = (const char *) glGetString( GL_VERSION );
    INFO_LOG( "opengl version: %s", gl_version );

    glfwSwapInterval( 1 );
    glViewport( 0, 0, intern.width, intern.height );

    glfwSetMouseButtonCallback( intern.window, handle_mouse_button );

    return 0;
}

void hardware_destroy()
{
    glfwDestroyWindow( intern.window );
    glfwTerminate();
}

using loop_function_t = void ( * )();

void hardware_set_loop( loop_function_t step )
{
    while ( !glfwWindowShouldClose( intern.window ) ) {
        intern.pending_event_count = 0;
        glfwPollEvents();
        step();
        glfwSwapBuffers( intern.window );
    }
}

int hardware_width()
{
    return intern.width;
}

int hardware_height()
{
    return intern.height;
}

float hardware_time()
{
    return glfwGetTime();
}

void hardware_rumble()
{
}

int * hardware_events( int * out_count )
{
    *out_count = intern.pending_event_count;
    return intern.pending_event_list;
}

int hardware_touch_pos_x()
{
    double x, y;
    glfwGetCursorPos( intern.window, &x, &y );
    return (int) x;
}

int hardware_touch_pos_y()
{
    double x, y;
    glfwGetCursorPos( intern.window, &x, &y );
    return (int) y;
}

int hardware_touch_down()
{
    return glfwGetMouseButton( intern.window, GLFW_MOUSE_BUTTON_LEFT ) ==
           GLFW_PRESS;
}
