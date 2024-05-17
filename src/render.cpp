#include "render.hpp"

#include "color.hpp"
#include "font.hpp"
#include "hardware.hpp"
#include "render_utils.hpp"
#include "res.hpp"
#include "shape.hpp"
#include "state.hpp"
#include "utils.hpp"

#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>

#ifdef __EMSCRIPTEN__
#include <GLES2/gl2.h>
#else
#include <glad/glad.h>
#endif

#include <math.h>

struct spritesheet_t {
    int texture;
    int cell_w;
    int cell_h;
    int width;
    int height;
    int padding;

    rect_t uv_rect( int x, int y )
    {
        rect_t r;
        r.x = (float) ( padding + x * ( cell_w + padding ) ) / width;
        r.y = (float) ( padding + y * ( cell_h + padding ) ) / height;
        r.w = (float) cell_w / width;
        r.h = (float) cell_h / height;
        return r;
    }
};

struct textured_sprite_t {
    rect_t rect;
    rect_t uv_rect;
    color_t color;
    float alpha = 1.0f;
    float rotation = 0.0f;

    int texture;
    void render();

    void from_sheet( spritesheet_t sheet, int sheet_x, int sheet_y );
};

struct solid_sprite_t {
    rect_t rect;
    color_t color;
    float alpha = 1.0f;
    float rotation = 0.0f;

    void render();
};

// render state
struct {
    vbuffer_t quad_pos_buffer;
    vbuffer_t quad_uv_buffer;

    vbuffer_t fb_pos_buffer;
    vbuffer_t fb_uv_buffer;

    vbuffer_t player_buffer;

    // Bullets
    vbuffer_t bitch_buffer;

    struct {
        int id;
        int proj;
        int model;
        int color;
    } shader1;

    struct {
        int id;
        int texture;
        int amount;
    } shader2;

    struct {
        int id;
        int proj;
        int model;
        int color;
        int texture;
    } shader3;

    font_t font;
    int font_texture;

    mat4 model;
    mat4 proj;

} intern;

static void init_shader1()
{
    int id = build_shader(
        find_shader_string( "shader1_vertex" ),
        find_shader_string( "shader1_fragment" )
    );
    intern.shader1.id = id;
    intern.shader1.proj = find_uniform( id, "u_proj" );
    intern.shader1.model = find_uniform( id, "u_model" );
    intern.shader1.color = find_uniform( id, "u_color" );

    glBindAttribLocation( id, 0, "a_pos" );
}

static void init_shader2()
{
    int id = build_shader(
        find_shader_string( "shader2_vertex" ),
        find_shader_string( "shader2_fragment" )
    );
    intern.shader2.id = id;
    intern.shader2.texture = find_uniform( id, "u_texture" );
    intern.shader2.amount = find_uniform( id, "u_amount" );

    glBindAttribLocation( id, 0, "a_pos" );
    glBindAttribLocation( id, 1, "a_uv" );
}

static void init_shader3()
{
    int id = build_shader(
        find_shader_string( "shader3_vertex" ),
        find_shader_string( "shader3_fragment" )
    );
    intern.shader3.id = id;
    intern.shader3.proj = find_uniform( id, "u_proj" );
    intern.shader3.model = find_uniform( id, "u_model" );
    intern.shader3.color = find_uniform( id, "u_color" );
    intern.shader3.texture = find_uniform( id, "u_texture" );

    glBindAttribLocation( id, 0, "a_pos" );
    glBindAttribLocation( id, 1, "a_uv" );
}

void compute_model_matrix( mat4 out, rect_t rect, float rotation )
{
    vec3 scale;
    scale[ 0 ] = roundf( rect.w );
    scale[ 1 ] = roundf( rect.h );
    scale[ 2 ] = 1.0f;

    vec3 translate;
    translate[ 0 ] = roundf( rect.x );
    translate[ 1 ] = roundf( rect.y );
    translate[ 2 ] = 0.0f;

    vec3 axis;
    axis[ 0 ] = 0.0f;
    axis[ 1 ] = 0.0f;
    axis[ 2 ] = 1.0f;

    glm_mat4_identity( out );
    glm_translate( out, translate );
    glm_scale( out, scale );
    vec3 temp;
    temp[ 0 ] = 0.5f;
    temp[ 1 ] = 0.5f;
    temp[ 2 ] = 0.0f;
    glm_translate( out, temp );
    glm_rotate( out, rotation, axis );
    temp[ 0 ] = -0.5f;
    temp[ 1 ] = -0.5f;
    temp[ 2 ] = 0.0f;
    glm_translate( out, temp );

    glm_mat4_mul( intern.model, out, out );
}

void textured_sprite_t::from_sheet(
    spritesheet_t sheet,
    int sheet_x,
    int sheet_y
)
{
    texture = sheet.texture;
    uv_rect = sheet.uv_rect( sheet_x, sheet_y );
}

void textured_sprite_t::render()
{
    vec4 color4;
    color4[ 0 ] = color.r;
    color4[ 1 ] = color.g;
    color4[ 2 ] = color.b;
    color4[ 3 ] = alpha;

    mat4 local_model;
    compute_model_matrix( local_model, rect, rotation );

    float uv_data[ 12 ];
    uv_rect.vertices_2d( uv_data );
    intern.quad_uv_buffer.set( uv_data, 6 );

    glUseProgram( intern.shader3.id );
    glBindTexture( GL_TEXTURE_2D, texture );
    set_uniform( intern.shader3.proj, intern.proj );
    set_uniform( intern.shader3.model, local_model );
    set_uniform( intern.shader3.color, color4 );
    set_uniform( intern.shader3.texture, 0 );
    intern.quad_pos_buffer.enable( 0 );
    intern.quad_uv_buffer.enable( 1 );

    glDrawArrays( GL_TRIANGLES, 0, intern.quad_pos_buffer.element_count );
}

void solid_sprite_t::render()
{
    vec4 color4;
    color4[ 0 ] = color.r;
    color4[ 1 ] = color.g;
    color4[ 2 ] = color.b;
    color4[ 3 ] = alpha;

    mat4 local_model;
    compute_model_matrix( local_model, rect, rotation );

    glUseProgram( intern.shader1.id );
    set_uniform( intern.shader1.proj, intern.proj );
    set_uniform( intern.shader1.model, local_model );
    set_uniform( intern.shader1.color, color4 );

    //intern.quad_pos_buffer.enable( 0 );
    intern.player_buffer.enable( 0 );

    glDrawArrays( GL_TRIANGLE_FAN, 0, intern.player_buffer.element_count );
}

static void render_bitch_bullet() {
    color_t color = color_yellow;

    vec4 color4;
    color4[ 0 ] = color.r;
    color4[ 1 ] = color.g;
    color4[ 2 ] = color.b;
    color4[ 3 ] = 1.0f;

    rect_t rect;
    rect.x = 300;
    rect.y = 300;
    rect.w = 10;
    rect.h = 10;

    float rotation = state.render_time;

    mat4 local_model;
    compute_model_matrix( local_model, rect, rotation );

    glUseProgram( intern.shader1.id );
    set_uniform( intern.shader1.proj, intern.proj );
    set_uniform( intern.shader1.model, local_model );
    set_uniform( intern.shader1.color, color4 );

    //intern.quad_pos_buffer.enable( 0 );
    intern.bitch_buffer.enable( 0 );

    glDrawArrays( GL_TRIANGLE_FAN, 0, intern.bitch_buffer.element_count );
}

static void setup_camera()
{
    glm_ortho(
        0.0f,
        hardware_width(),
        hardware_height(),
        0.0f,
        0.0f,
        1000.0f,
        intern.proj
    );
}

enum alignment_t {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
};

struct text_settings_t {
    color_t color = color_white;
    float alpha = 1.0f;
    alignment_t align_x = ALIGN_LEFT;
    alignment_t align_y = ALIGN_LEFT;
    bool render_bg = false;
    float scale = 1.0f;
};

static void render_text(
    int x,
    int y,
    const char * str,
    text_settings_t settings = text_settings_t()
)
{
    glyph_t g_list[ 64 ];
    int g_count;
    float width;
    float height;

    intern.font.calc_glyphs( g_list, &g_count, &width, &height, str );

    // x -> s * x    s * x - x = x * (s - 1)

    width *= settings.scale;
    height *= settings.scale;

    if ( settings.align_x == ALIGN_RIGHT ) {
        x -= width;
    }

    if ( settings.align_x == ALIGN_CENTER ) {
        x -= width * 0.5f;
    }

    if ( settings.align_y == ALIGN_RIGHT ) {
        y -= height;
    }

    if ( settings.align_y == ALIGN_CENTER ) {
        y -= height * 0.5f;
    }

    if ( settings.render_bg ) {
        solid_sprite_t solid;
        solid.rect = { (float) x, (float) y, width, height };
        solid.rect.margin( -3 );
        solid.color = color_black;
        solid.render();
    }

    vec3 scale;
    scale[ 0 ] = settings.scale;
    scale[ 1 ] = settings.scale;
    scale[ 2 ] = 1.0f;

    vec3 translate;
    translate[ 0 ] = x;
    translate[ 1 ] = y;
    translate[ 2 ] = 0.0f;

    glm_mat4_identity( intern.model );
    glm_translate( intern.model, translate );
    glm_scale( intern.model, scale );

    for ( int i = 0; i < g_count; i++ ) {
        textured_sprite_t s;
        s.rect.x = g_list[ i ].x;
        s.rect.y = g_list[ i ].y;
        s.rect.w = g_list[ i ].w;
        s.rect.h = g_list[ i ].h;
        s.uv_rect.x = g_list[ i ].u1;
        s.uv_rect.y = g_list[ i ].v1;
        s.uv_rect.w = g_list[ i ].u2 - g_list[ i ].u1;
        s.uv_rect.h = g_list[ i ].v2 - g_list[ i ].v1;
        s.texture = intern.font_texture;
        s.color = settings.color;
        s.alpha = settings.alpha;
        s.render();
    }

    glm_mat4_identity( intern.model );
}

void render_init()
{
    float quad_data[ 12 ];
    rect_t{ 0.0f, 0.0f, 1.0f, 1.0f }.vertices_2d( quad_data );

    float fb_pos_data[ 12 ];
    rect_t{ -1.0f, -1.0f, 2.0f, 2.0f }.vertices_2d( fb_pos_data );

    float fb_uv_data[ 12 ];
    rect_t{ 0.0f, 0.0f, 1.0f, 1.0f }.vertices_2d( fb_uv_data );

    float player_data[ 18 ];
    ngon_vertices( player_data, 7 );

    float bitch_bullet[ 10 ];
    ngon_vertices(bitch_bullet, 3); // (amount + 2) * 2

    // init vertex buffers

    intern.quad_pos_buffer.init( 2 );
    intern.quad_pos_buffer.set( quad_data, 6 );

    intern.quad_uv_buffer.init( 2 );
    intern.quad_uv_buffer.set( fb_uv_data, 6 );

    intern.player_buffer.init( 2 );
    intern.player_buffer.set( player_data, 9 );

    intern.bitch_buffer.init(2);
    intern.bitch_buffer.set(bitch_bullet, 5);

    // intern.fb_pos_buffer.init( 2 );
    // intern.fb_pos_buffer.set( fb_pos_data, 6 );

    // intern.fb_uv_buffer.init( 2 );
    // intern.fb_uv_buffer.set( fb_uv_data, 6 );

    // init shaders

    init_shader1();
    init_shader2();
    init_shader3();

    // init font

    intern.font.init( find_res( "bit.fnt" ) );
    intern.font_texture = load_texture( find_res( "bit.png" ) );

    // init misc

    glm_mat4_identity( intern.model );
    setup_camera();

    // init gl state

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // blend alpha
}

void render()
{
    setup_camera();

    glClearColor( 0.2f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    solid_sprite_t s;
    s.rect.x = state.player_pos[ 0 ];
    s.rect.y = state.player_pos[ 1 ];
    s.rect.w = 32;
    s.rect.h = 32;
    s.rect.centerize();

    s.color = color_white;
    s.render();

    render_bitch_bullet();
}
