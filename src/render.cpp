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
#include <cglm/vec2.h>

#ifdef __EMSCRIPTEN__
#include <GLES2/gl2.h>
#else
#include <glad/glad.h>
#endif

#include <math.h>

struct textured_sprite_t {
    rect_t rect;
    rect_t uv_rect;
    color_t color;
    float alpha = 1.0f;

    int texture;
    void render();
};

struct solid_sprite_t {
    rect_t rect;
    color_t color;
    float alpha = 1.0f;

    void render();
};

// render state
struct {
    vbuffer_t quad_pos_buffer;
    vbuffer_t quad_uv_buffer;

    vbuffer_t fb_pos_buffer;
    vbuffer_t fb_uv_buffer;

    vbuffer_t player_buffer;

    // bullets
    vbuffer_t bitch_buffer;
    vbuffer_t line_bullet_buffer;

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

void compute_model_matrix( mat4 out, rect_t rect )
{
    vec3 scale;
    scale[ 0 ] = roundf( rect.w );
    scale[ 1 ] = roundf( rect.h );
    scale[ 2 ] = 1.0f;

    glm_mat4_identity( out );
    glm_translate_x( out, roundf( rect.x ) );
    glm_translate_y( out, roundf( rect.y ) );
    glm_scale( out, scale );
    glm_mat4_mul( intern.model, out, out );
}

void textured_sprite_t::render()
{
    vec4 color4;
    color4[ 0 ] = color.r;
    color4[ 1 ] = color.g;
    color4[ 2 ] = color.b;
    color4[ 3 ] = alpha;

    mat4 local_model;
    compute_model_matrix( local_model, rect );

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

struct sprite_t {
    vec2 pos;
    float scale;
    float rotation = 0.0f;

    color_t color;
    float alpha = 1.0f;

    void setup();
};

void sprite_t::setup()
{
    vec4 color4;
    color4[ 0 ] = color.r;
    color4[ 1 ] = color.g;
    color4[ 2 ] = color.b;
    color4[ 3 ] = alpha;

    mat4 local_model;
    glm_mat4_identity( local_model );
    glm_translate_x( local_model, pos[ 0 ] );
    glm_translate_y( local_model, pos[ 1 ] );
    glm_scale_uni( local_model, scale );
    glm_rotate_z( local_model, rotation, local_model );

    glUseProgram( intern.shader1.id );
    set_uniform( intern.shader1.proj, intern.proj );
    set_uniform( intern.shader1.model, local_model );
    set_uniform( intern.shader1.color, color4 );
}

void solid_sprite_t::render()
{
    vec4 color4;
    color4[ 0 ] = color.r;
    color4[ 1 ] = color.g;
    color4[ 2 ] = color.b;
    color4[ 3 ] = alpha;

    mat4 local_model;
    compute_model_matrix( local_model, rect );

    glUseProgram( intern.shader1.id );
    set_uniform( intern.shader1.proj, intern.proj );
    set_uniform( intern.shader1.model, local_model );
    set_uniform( intern.shader1.color, color4 );

    intern.quad_pos_buffer.enable( 0 );
    glDrawArrays( GL_TRIANGLES, 0, intern.quad_pos_buffer.element_count );
}

static void render_player()
{
    sprite_t s;
    glm_vec2_copy( state.player_pos, s.pos );
    s.scale = 8.0f + state.player_z * 20;
    s.color = state.player_z ? color_green : color_white;
    s.rotation = state.render_time;
    s.setup();

    intern.player_buffer.enable( 0 );
    glDrawArrays( GL_TRIANGLE_FAN, 0, intern.player_buffer.element_count );
}

static void setup_ui_camera()
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

static void setup_world_camera()
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

    glm_translate_x(
        intern.proj,
        hardware_width() * 0.5f - state.player_pos[ 0 ]
    );
    glm_translate_y(
        intern.proj,
        hardware_height() * 0.5f - state.player_pos[ 1 ]
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

    float player_data[ 14 ];
    ngon_vertices( player_data, 5 );

    float bitch_bullet[ 10 ];
    ngon_vertices( bitch_bullet, 3 ); // (amount + 2) * 2

    // init vertex buffers

    intern.quad_pos_buffer.init( 2 );
    intern.quad_pos_buffer.set( quad_data, 6 );

    intern.quad_uv_buffer.init( 2 );
    intern.quad_uv_buffer.set( fb_uv_data, 6 );

    intern.player_buffer.init( 2 );
    intern.player_buffer.set( player_data, 7 );

    intern.bitch_buffer.init( 2 );
    intern.bitch_buffer.set( bitch_bullet, 5 );

    intern.line_bullet_buffer.init( 2 );

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

    // init gl state

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // blend alpha
}

////////////////////////////////////////////////////////////////////////////////

static void render_bitch_bullet( int i )
{
    sprite_t s;
    s.pos[ 0 ] = state.bullet_pos_list[ i ][ 0 ];
    s.pos[ 1 ] = state.bullet_pos_list[ i ][ 1 ];
    s.scale = 5.0f;
    s.color = color_red;
    s.rotation = state.render_time;
    s.setup();

    intern.bitch_buffer.enable( 0 );
    glDrawArrays( GL_TRIANGLE_FAN, 0, intern.bitch_buffer.element_count );
}

static void render_line_bullet( int i )
{
    vec2 delta;
    glm_vec2_sub(
        state.line_bullet_pos2_list[ i ],
        state.line_bullet_pos1_list[ i ],
        delta
    );

    sprite_t s;
    s.pos[ 0 ] = state.line_bullet_pos1_list[ i ][ 0 ];
    s.pos[ 1 ] = state.line_bullet_pos1_list[ i ][ 1 ];
    s.scale = 1.0f;
    s.color = color_yellow;
    s.rotation = atan2f( delta[ 1 ], delta[ 0 ] );
    s.setup();

    float len = glm_vec2_norm( delta );
    float w = 3.0f;

    // heh... shur up
    float line_bullet[ 16 ];
    line_bullet[ 0 ] = 0.0f;
    line_bullet[ 1 ] = 0.0f;
    line_bullet[ 2 ] = -w;
    line_bullet[ 3 ] = 0.0f;
    line_bullet[ 4 ] = 0.0f;
    line_bullet[ 5 ] = -w;
    line_bullet[ 6 ] = len;
    line_bullet[ 7 ] = -w;
    line_bullet[ 8 ] = len + w;
    line_bullet[ 9 ] = 0.0f;
    line_bullet[ 10 ] = len;
    line_bullet[ 11 ] = w;
    line_bullet[ 12 ] = 0.0f;
    line_bullet[ 13 ] = w;
    line_bullet[ 14 ] = -w;
    line_bullet[ 15 ] = 0.0f;

    intern.line_bullet_buffer.set( line_bullet, 8 );
    intern.line_bullet_buffer.enable( 0 );
    glDrawArrays( GL_TRIANGLE_FAN, 0, intern.line_bullet_buffer.element_count );
}

static void render_ui()
{
    setup_ui_camera();

    text_settings_t settings;
    settings.scale = 5.0f;

    render_text( 0, 0, "meow", settings );
}

static void render_room_outline( int i )
{
    solid_sprite_t s;
    s.rect = state.room_rect_list[ i ];
    s.rect.margin( -10 );
    s.color = color_white;
    s.render();
}

static void render_room( int i )
{
    solid_sprite_t s;
    s.rect = state.room_rect_list[ i ];
    s.rect.margin( -5 );
    s.color = color_black;
    s.render();
}

static void render_rooms()
{
    for ( int i = 0; i < state.room_count; i++ ) {
        render_room_outline( i );
    }

    for ( int i = 0; i < state.room_count; i++ ) {
        render_room( i );
    }
}

static void render_world()
{
    setup_world_camera();

    render_rooms();

    for ( int i = 0; i < state.bullet_count; i++ ) {
        render_bitch_bullet( i );
    }

    for ( int i = 0; i < state.line_bullet_count; i++ ) {
        render_line_bullet( i );
    }

    render_player();
}

void render()
{
    glClearColor( 0.2f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    render_world();
    render_ui();
}
