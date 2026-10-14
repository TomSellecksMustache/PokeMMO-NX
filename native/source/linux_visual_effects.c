// Optional GL 2.1 / GLSL 1.20 presentation filter. We process a COPY of the
// final backbuffer; game-owned textures, FBOs, shaders and UI are untouched.
// This runs before NXpanded's native overlays so their text stays sharp.
#include "linux_visual_effects.h"
#include <EGL/egl.h>
#include <stdatomic.h>
#include <string.h>

// Function pointers are resolved from the same Mesa/EGL context as the game.
typedef unsigned int GLenum, GLuint, GLbitfield;
typedef int GLint, GLsizei;
typedef float GLfloat;
typedef char GLchar;
enum {
    GL_FRAGMENT_SHADER = 0x8B30, GL_VERTEX_SHADER = 0x8B31,
    GL_COMPILE_STATUS = 0x8B81, GL_LINK_STATUS = 0x8B82,
    GL_TEXTURE_2D = 0x0DE1, GL_TEXTURE0 = 0x84C0,
    GL_TEXTURE_MIN_FILTER = 0x2801, GL_TEXTURE_MAG_FILTER = 0x2800,
    GL_TEXTURE_WRAP_S = 0x2802, GL_TEXTURE_WRAP_T = 0x2803,
    GL_NEAREST = 0x2600, GL_LINEAR = 0x2601, GL_CLAMP_TO_EDGE = 0x812F,
    GL_RGBA = 0x1908, GL_UNSIGNED_BYTE = 0x1401,
    GL_CURRENT_PROGRAM = 0x8B8D, GL_FRAMEBUFFER = 0x8D40, GL_FRAMEBUFFER_BINDING = 0x8CA6,
    GL_ACTIVE_TEXTURE = 0x84E0, GL_MATRIX_MODE = 0x0BA0,
    GL_BACK = 0x0405, GL_READ_BUFFER = 0x0C02,
    GL_DEPTH_TEST = 0x0B71, GL_STENCIL_TEST = 0x0B90, GL_CULL_FACE = 0x0B44,
    GL_SCISSOR_TEST = 0x0C11, GL_BLEND = 0x0BE2, GL_LIGHTING = 0x0B50,
    GL_ALPHA_TEST = 0x0BC0, GL_FOG = 0x0B60,
    GL_TEXTURE = 0x1702, GL_PROJECTION = 0x1701, GL_MODELVIEW = 0x1700,
    GL_QUADS = 0x0007, GL_ALL_ATTRIB_BITS = 0x000FFFFF
};
static struct {
    GLuint (*CreateShader)(GLenum);
    void (*ShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
    void (*CompileShader)(GLuint);
    void (*GetShaderiv)(GLuint, GLenum, GLint *);
    void (*DeleteShader)(GLuint);
    GLuint (*CreateProgram)(void);
    void (*AttachShader)(GLuint, GLuint);
    void (*LinkProgram)(GLuint);
    void (*GetProgramiv)(GLuint, GLenum, GLint *);
    void (*DeleteProgram)(GLuint);
    void (*UseProgram)(GLuint);
    GLint (*GetUniformLocation)(GLuint, const GLchar *);
    void (*Uniform1i)(GLint, GLint);
    void (*Uniform2f)(GLint, GLfloat, GLfloat);
    void (*GetIntegerv)(GLenum, GLint *);
    GLenum (*GetError)(void);
    void (*GenTextures)(GLsizei, GLuint *);
    void (*DeleteTextures)(GLsizei, const GLuint *);
    void (*BindTexture)(GLenum, GLuint);
    void (*TexParameteri)(GLenum, GLenum, GLint);
    void (*TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
    void (*CopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    void (*ReadBuffer)(GLenum);
    void (*ActiveTexture)(GLenum);
    void (*BindFramebuffer)(GLenum, GLuint);
    void (*Viewport)(GLint, GLint, GLsizei, GLsizei);
    void (*PushAttrib)(GLbitfield);
    void (*PopAttrib)(void);
    void (*MatrixMode)(GLenum);
    void (*PushMatrix)(void);
    void (*PopMatrix)(void);
    void (*LoadIdentity)(void);
    void (*Disable)(GLenum);
    void (*Enable)(GLenum);
    void (*Color4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (*Begin)(GLenum);
    void (*TexCoord2f)(GLfloat, GLfloat);
    void (*Vertex2f)(GLfloat, GLfloat);
    void (*End)(void);
} gl;
static GLuint program, captured;
static unsigned texture_width, texture_height;
static GLint texel_uniform, scene_uniform, effect_uniform[VISUAL_EFFECT_COUNT], extra_uniform;
static atomic_int status; // 0 before first active frame, 1 ready, -1 unavailable

static const char *const vertex_source =
    "#version 120\n"
    "void main() { gl_Position = ftransform(); gl_TexCoord[0] = gl_MultiTexCoord0; }\n";
static const char *const fragment_source =
    "#version 120\n"
    "uniform sampler2D scene;\n"
    "uniform vec2 pixel;\n"
    "uniform int tilt, bloom, grade, soften, vignette, fxaa, vibrance, contrast, highlights, sharpen, shadows, haze;\n"
    "uniform int wideGlow, focus, painter;\n"
    "uniform int u13;\n"
    "void main() {\n"
    "  vec2 uv = gl_TexCoord[0].xy;\n"
    "  vec3 base = texture2D(scene, uv).rgb;\n"
    "  vec3 color = base;\n"
    "  vec3 blur = base;\n"
    "  if (soften != 0) {\n"
    "    vec2 d = pixel * 0.85;\n"
    "    vec3 sm = (texture2D(scene, uv+vec2(d.x,d.y)).rgb +\n"
    "               texture2D(scene, uv+vec2(-d.x,d.y)).rgb +\n"
    "               texture2D(scene, uv+vec2(d.x,-d.y)).rgb +\n"
    "               texture2D(scene, uv-vec2(d.x,d.y)).rgb) * 0.25;\n"
    "    color = mix(color, sm, 0.28);\n"
    "  }\n"
    "  if (fxaa != 0 || sharpen != 0) {\n"
    "    vec3 a = texture2D(scene, uv+vec2(pixel.x,0.0)).rgb;\n"
    "    vec3 b = texture2D(scene, uv-vec2(pixel.x,0.0)).rgb;\n"
    "    vec3 c = texture2D(scene, uv+vec2(0.0,pixel.y)).rgb;\n"
    "    vec3 d = texture2D(scene, uv-vec2(0.0,pixel.y)).rgb;\n"
    "    vec3 avg = (a+b+c+d)*0.25;\n"
    "    if (fxaa != 0) {\n"
    "      float contrastEdge = abs(dot(base-avg, vec3(0.299,0.587,0.114)));\n"
    "      color = mix(color, avg, smoothstep(0.045,0.20,contrastEdge)*0.32);\n"
    "    }\n"
    "    if (sharpen != 0) color = clamp(color+(base-avg)*0.30,0.0,1.0);\n"
    "  }\n"
    "  if (tilt != 0 || bloom != 0 || u13 != 0) {\n"
    "    vec2 dx = vec2(pixel.x*2.8,0.0);\n"
    "    vec2 dy = vec2(0.0,pixel.y*2.8);\n"
    "    blur = (texture2D(scene,uv+dx).rgb + texture2D(scene,uv-dx).rgb +\n"
    "            texture2D(scene,uv+dy).rgb + texture2D(scene,uv-dy).rgb +\n"
    "            texture2D(scene,uv+dx+dy).rgb + texture2D(scene,uv-dx+dy).rgb +\n"
    "            texture2D(scene,uv+dx-dy).rgb + texture2D(scene,uv-dx-dy).rgb)*0.125;\n"
    "    if (tilt != 0) {\n"
    "      float edge = smoothstep(0.14,0.41,abs(uv.y-0.53));\n"
    "      color = mix(color,blur,edge*0.78);\n"
    "    }\n"
    "    if (bloom != 0) color += max(blur-vec3(0.62),vec3(0.0))*0.30;\n"
    "  }\n"
    "  if (wideGlow != 0 || focus != 0 || painter != 0) {\n"
    "    vec3 softened = base * 2.0;\n"
    "    vec3 rim = vec3(0.0);\n"
    "    vec3 weighted = base * 2.0;\n"
    "    float sumWeight = 2.0;\n"
    "    vec3 tap;\n"
    "    float weight;\n"
    "    tap = texture2D(scene, uv + pixel * vec2(6.0,0.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(-6.0,0.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(0.0,6.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(0.0,-6.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(4.0,4.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(-4.0,4.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(4.0,-4.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(-4.0,-4.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(11.0,0.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(-11.0,0.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(0.0,11.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    tap = texture2D(scene, uv + pixel * vec2(0.0,-11.0)).rgb;\n"
    "    softened += tap;\n"
    "    if (wideGlow != 0) rim += max(tap - vec3(0.68), vec3(0.0));\n"
    "    if (painter != 0) {\n"
    "      weight = exp(-dot(tap-base,tap-base)*18.0);\n"
    "      weighted += weight*tap; sumWeight += weight;\n"
    "    }\n"
    "    softened *= 1.0/14.0;\n"
    "    if (focus != 0) {\n"
    "      float offFocus = smoothstep(0.10,0.38,abs(uv.y-0.50));\n"
    "      color = mix(color,softened,offFocus*0.81);\n"
    "    }\n"
    "    if (painter != 0) color = mix(color,weighted/max(sumWeight,0.0001),0.78);\n"
    "    if (wideGlow != 0) color += rim*(0.75/12.0);\n"
    "  }\n"
    "  if (grade != 0) color = pow(clamp(color,0.0,1.0),vec3(0.95,1.0,1.05));\n"
    "  float luma = dot(color,vec3(0.2126,0.7152,0.0722));\n"
    "  if (vibrance != 0) color = mix(vec3(luma),color,1.12);\n"
    "  if (contrast != 0) color = (color-vec3(0.5))*1.11+vec3(0.5);\n"
    "  if (shadows != 0) color *= mix(0.83,1.0,smoothstep(0.10,0.62,luma));\n"
    "  if (highlights != 0) color = color/(vec3(1.0)+max(color-vec3(0.65),vec3(0.0))*0.32);\n"
    "  if (haze != 0) color = mix(color,vec3(0.68,0.73,0.70),0.045+0.06*uv.y);\n"
    "  if (u13 != 0) {\n"
    "    // A single additional treatment on the same captured frame.\n"
    "    float lum = dot(color,vec3(0.2126,0.7152,0.0722));\n"
    "    vec3 shade = mix(vec3(0.48,0.61,0.75),vec3(1.22,1.11,0.98),smoothstep(0.14,0.78,lum));\n"
    "    color *= shade;\n"
    "    color = mix(color,blur,smoothstep(0.10,0.43,abs(uv.y-0.51))*0.34+0.08);\n"
    "    color += max(blur-vec3(0.62),vec3(0.0))*0.34;\n"
    "    color = (color-vec3(0.5))*1.10+vec3(0.5);\n"
    "  }\n"
    "  if (vignette != 0 || u13 != 0) {\n"
    "    vec2 p = (uv-vec2(0.5))*vec2(1.05,0.91);\n"
    "    color *= 1.0-smoothstep(0.23,0.72,length(p))*(vignette != 0 ? 0.24 : 0.16);\n"
    "  }\n"
    "  gl_FragColor = vec4(clamp(color,0.0,1.0),1.0);\n"
    "}\n";

static bool symbol(void *slot, const char *name) {
    void *address = (void *)eglGetProcAddress(name);
    if (address) memcpy(slot, &address, sizeof(address));
    return address != NULL;
}
static bool load(void) {
#define LOAD(name) if (!symbol(&gl.name, "gl" #name)) return false
    LOAD(CreateShader); LOAD(ShaderSource); LOAD(CompileShader); LOAD(GetShaderiv); LOAD(DeleteShader);
    LOAD(CreateProgram); LOAD(AttachShader); LOAD(LinkProgram); LOAD(GetProgramiv); LOAD(DeleteProgram);
    LOAD(UseProgram); LOAD(GetUniformLocation); LOAD(Uniform1i); LOAD(Uniform2f); LOAD(GetIntegerv); LOAD(GetError);
    LOAD(GenTextures); LOAD(DeleteTextures); LOAD(BindTexture); LOAD(TexParameteri); LOAD(TexImage2D);
    LOAD(CopyTexSubImage2D); LOAD(ReadBuffer); LOAD(ActiveTexture); LOAD(BindFramebuffer); LOAD(Viewport);
    LOAD(PushAttrib); LOAD(PopAttrib); LOAD(MatrixMode); LOAD(PushMatrix); LOAD(PopMatrix); LOAD(LoadIdentity);
    LOAD(Disable); LOAD(Enable); LOAD(Color4f); LOAD(Begin); LOAD(TexCoord2f); LOAD(Vertex2f); LOAD(End);
#undef LOAD
    return true;
}
static GLuint compile(GLenum type, const char *source) {
    GLuint shader = gl.CreateShader(type);
    if (!shader) return 0;
    gl.ShaderSource(shader, 1, (const GLchar *const *)&source, NULL);
    gl.CompileShader(shader);
    GLint ok = 0;
    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return shader;
    gl.DeleteShader(shader);
    return 0;
}
static bool initialize(void) {
    if (!load()) return false;
    GLuint vs = compile(GL_VERTEX_SHADER, vertex_source);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragment_source);
    if (!vs || !fs) {
        if (vs) gl.DeleteShader(vs);
        if (fs) gl.DeleteShader(fs);
        return false;
    }
    program = gl.CreateProgram();
    if (program) {
        gl.AttachShader(program, vs);
        gl.AttachShader(program, fs);
        gl.LinkProgram(program);
    }
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    GLint ok = 0;
    if (program) gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        if (program) gl.DeleteProgram(program);
        program = 0;
        return false;
    }
    texel_uniform = gl.GetUniformLocation(program, "pixel");
    scene_uniform = gl.GetUniformLocation(program, "scene");
    static const char *const uniforms[] = {
        "tilt", "bloom", "grade", "soften", "vignette", "fxaa", "vibrance",
        "contrast", "highlights", "sharpen", "shadows", "haze",
        "wideGlow", "focus", "painter"
    };
    for (unsigned i = 0; i < VISUAL_EFFECT_COUNT; ++i)
        effect_uniform[i] = gl.GetUniformLocation(program, uniforms[i]);
    extra_uniform = gl.GetUniformLocation(program, "u13");
    gl.GenTextures(1, &captured);
    return captured != 0;
}
int linuxVisualEffectsStatus(void) { return atomic_load(&status); }
void linuxVisualEffectsRender(unsigned width, unsigned height) {
    uint32_t mask = linuxVisualEffectMask();
    if (!(mask || linuxVisualEffectsPulseActive()) || !width || !height || width > 3840 || height > 2160 || atomic_load(&status) < 0) return;
    if (!atomic_load(&status)) {
        if (!initialize()) { atomic_store(&status, -1); return; }
        atomic_store(&status, 1);
    }
    GLint previous_program = 0, previous_fbo = 0, previous_unit = GL_TEXTURE0, previous_matrix = GL_MODELVIEW;
    gl.GetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
    gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &previous_unit);
    gl.GetIntegerv(GL_MATRIX_MODE, &previous_matrix);
    gl.PushAttrib(GL_ALL_ATTRIB_BITS);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0); // always sample the *display*, not a game-owned render target
    gl.ReadBuffer(GL_BACK);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindTexture(GL_TEXTURE_2D, captured);
    if (texture_width != width || texture_height != height) {
        // Allocate once per resize, not once per frame.
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        texture_width = width;
        texture_height = height;
    }
    // No readback to the CPU. Copy the default backbuffer to a private GPU texture.
    gl.CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLsizei)width, (GLsizei)height);
    // If capture/allocation fails, do NOT draw a replacement frame.
    bool valid_capture = gl.GetError() == 0;
    if (!valid_capture) atomic_store(&status, -1);
    if (valid_capture) {
        if (mask & (1u << VISUAL_SOFT_PIXELS)) {
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } else {
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        gl.Viewport(0, 0, (GLsizei)width, (GLsizei)height);
        const GLenum off[] = {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_BLEND, GL_LIGHTING, GL_ALPHA_TEST, GL_FOG};
        for (unsigned i = 0; i < sizeof(off) / sizeof(off[0]); ++i) gl.Disable(off[i]);
        gl.Enable(GL_TEXTURE_2D);
        gl.MatrixMode(GL_TEXTURE); gl.PushMatrix(); gl.LoadIdentity();
        gl.MatrixMode(GL_PROJECTION); gl.PushMatrix(); gl.LoadIdentity();
        gl.MatrixMode(GL_MODELVIEW); gl.PushMatrix(); gl.LoadIdentity();
        gl.UseProgram(program);
        gl.Uniform1i(scene_uniform, 0);
        gl.Uniform2f(texel_uniform, 1.0f / (float)width, 1.0f / (float)height);
        for (unsigned i = 0; i < VISUAL_EFFECT_COUNT; ++i)
            gl.Uniform1i(effect_uniform[i], !!(mask & (1u << i)));
        gl.Uniform1i(extra_uniform, linuxVisualEffectsPulseActive() ? 1 : 0);
        gl.Color4f(1, 1, 1, 1);
        gl.Begin(GL_QUADS);
        gl.TexCoord2f(0, 0); gl.Vertex2f(-1, -1);
        gl.TexCoord2f(1, 0); gl.Vertex2f( 1, -1);
        gl.TexCoord2f(1, 1); gl.Vertex2f( 1,  1);
        gl.TexCoord2f(0, 1); gl.Vertex2f(-1,  1);
        gl.End();
        gl.MatrixMode(GL_MODELVIEW); gl.PopMatrix();
        gl.MatrixMode(GL_PROJECTION); gl.PopMatrix();
        gl.MatrixMode(GL_TEXTURE); gl.PopMatrix();
    } // valid_capture
    gl.PopAttrib();
    gl.UseProgram((GLuint)previous_program);
    gl.BindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous_fbo);
    gl.ActiveTexture((GLenum)previous_unit);
    gl.MatrixMode((GLenum)previous_matrix);
}
void linuxVisualEffectsReset(void) {
    // The EGL context owns all GL objects; called only *after* it is destroyed.
    // The next new context will load its own symbols/shader/texture.
    memset(&gl, 0, sizeof(gl));
    program = captured = 0;
    texture_width = texture_height = 0;
    atomic_store(&status, 0);
}
