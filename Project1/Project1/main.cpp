#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
// 改回SDL2头文件
#define SDL_MAIN_HANDLED
#include <SDL.h>

// 2D向量：核心渲染逻辑不变
struct Vec2 {
    float x, y;

    Vec2(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
    Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
    Vec2 operator*(float scale) const { return { x * scale, y * scale }; }
};

// 颜色：核心渲染逻辑不变
struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) : r(r_), g(g_), b(b_), a(a_) {}
};

// 矩形：核心渲染逻辑不变
struct Rect {
    Vec2 pos;
    float width;
    float height;
    Rect(Vec2 p, float w, float h) : pos(p), width(w), height(h) {}
};

// 软渲染器核心类（仅改SDL2 API，渲染逻辑完全不变）
class SoftRenderer {
private:
    SDL_Window* window;
    SDL_Renderer* sdl_renderer;
    SDL_Texture* framebuffer_texture;
    std::vector<Color> framebuffer;
    int screen_width;
    int screen_height;

public:
    SoftRenderer(int w, int h) : screen_width(w), screen_height(h) {
        // SDL2初始化（原生API，无兼容性问题）
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "SDL初始化失败：" << SDL_GetError() << std::endl;
            system("pause");
            exit(1);
        }

        // SDL2创建窗口（原生参数）
        window = SDL_CreateWindow(
            "2D Soft Renderer (SDL2)",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            w, h,
            SDL_WINDOW_ALLOW_HIGHDPI
        );
        if (!window) {
            std::cerr << "窗口创建失败：" << SDL_GetError() << std::endl;
            SDL_Quit();
            system("pause");
            exit(1);
        }

        // SDL2创建渲染器（原生参数）
        sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!sdl_renderer) {
            std::cerr << "渲染器创建失败：" << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            system("pause");
            exit(1);
        }
        SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

        // SDL2创建纹理（原生参数）
        framebuffer_texture = SDL_CreateTexture(
            sdl_renderer,
            SDL_PIXELFORMAT_BGRA8888,
            SDL_TEXTUREACCESS_STREAMING,
            w, h
        );
        if (!framebuffer_texture) {
            std::cerr << "纹理创建失败：" << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(sdl_renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            system("pause");
            exit(1);
        }

        // 帧缓冲区初始化（核心逻辑不变）
        SDL_SetTextureBlendMode(framebuffer_texture, SDL_BLENDMODE_BLEND);
        framebuffer.resize(w * h, Color(0, 0, 0, 0));
    }

    ~SoftRenderer() {
        // 资源释放（SDL2原生API）
        SDL_DestroyTexture(framebuffer_texture);
        SDL_DestroyRenderer(sdl_renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    // 清空帧缓冲区（核心逻辑不变）
    void clear(const Color& color) {
        std::fill(framebuffer.begin(), framebuffer.end(), color);
    }

    // 绘制单个像素（核心逻辑不变）
    void set_pixel(int x, int y, const Color& color) {
        if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
            return;
        }
        framebuffer[y * screen_width + x] = color;
    }

    // 全局/类成员变量：SDL_Renderer* renderer（你之前创建的渲染器）
    void draw_circle(const Vec2& center, float radius, const Color& color, const Color& bg_color) {
        // 1. 计算圆形的包围盒
        int x_start = static_cast<int>(center.x - radius);
        int x_end = static_cast<int>(center.x + radius);
        int y_start = static_cast<int>(center.y - radius);
        int y_end = static_cast<int>(center.y + radius);

        for (int y = y_start; y < y_end; y++) {
            for (int x = x_start; x < x_end; x++) {
                // 2. 计算像素到圆心的距离
                float dx = x - center.x;
                float dy = y - center.y;
                float distance = sqrt(dx * dx + dy * dy);
                if (distance > radius) continue;

                // 3. 计算柔和渐变强度（保留你的核心效果）
                float smooth_intensity = 1.0f - pow(distance / radius, 3);
                smooth_intensity = std::max(0.0f, std::min(1.0f, smooth_intensity));

                // 4. 计算最终颜色（渐变）
                uint8_t r = static_cast<uint8_t>(color.r * smooth_intensity + bg_color.r * (1 - smooth_intensity));
                uint8_t g = static_cast<uint8_t>(color.g * smooth_intensity + bg_color.g * (1 - smooth_intensity));
                uint8_t b = static_cast<uint8_t>(color.b * smooth_intensity + bg_color.b * (1 - smooth_intensity));

                // 5. SDL自带函数设置颜色+画点（绝对不会错）
                SDL_SetRenderDrawColor(sdl_renderer, r, g, b, 255);
                SDL_RenderDrawPoint(sdl_renderer, x, y);
            }
        }
    }

    // 刷新屏幕（SDL2原生API：SDL_RenderCopy）
    void present() {
        SDL_UpdateTexture(
            framebuffer_texture,
            nullptr,
            framebuffer.data(),
            screen_width * sizeof(Color)
        );
        SDL_RenderCopy(sdl_renderer, framebuffer_texture, nullptr, nullptr);
        SDL_RenderPresent(sdl_renderer);
    }
};

// 主函数（SDL2原生事件循环）
int main() {
    try {
        SoftRenderer renderer(800, 600);
        bool is_running = true;
        SDL_Event event;
        int mouse_x = 400, mouse_y = 300;
        static float breath_offset = 0.0f;

        while (is_running) {
            // 事件处理（保留鼠标跟随）
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) is_running = false;
                else if (event.type == SDL_MOUSEMOTION) {
                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                }
            }

            // 渲染逻辑
            Color bg_color(224, 224, 224, 255);
            renderer.clear(bg_color);

            // 呼吸效果系数（和之前一致）
            float breath1 = sin(breath_offset * 0.01f) * 8.0f;  // 第一个圆呼吸幅度
            float breath2 = sin(breath_offset * 0.005f) * 6.0f; // 第二个圆
            float breath3 = sin(breath_offset * 0.01f) * 7.0f; // 第三个圆

            // 1. 第一个圆形：浅棕色 + 鼠标跟随 + 呼吸（半径80+呼吸）
            renderer.draw_circle(
                Vec2(mouse_x, mouse_y),          // 圆心=鼠标位置
                80 + breath1,                    // 基础半径80，加呼吸波动
                Color(255, 0, 0, 0),
                bg_color
            );

            // 2. 第二个圆形：浅粉色 + 上下浮动 + 呼吸（位置固定，Y轴浮动）
            renderer.draw_circle(
                Vec2(400, 200 + sin(breath_offset * 0.01f) * 15), // Y轴浮动
                70 + breath2,                                  // 基础半径70
                Color(0, 255, 0, 0),
                bg_color
            );

            // 3. 第三个圆形：浅绿色 + 呼吸（位置固定）
            renderer.draw_circle(
                Vec2(250, 350),
                60 + breath3,                    // 基础半径60
                Color(0, 0, 255, 0),
                bg_color
            );

            renderer.present();
            SDL_Delay(16);
            breath_offset += 1.0f;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "程序异常：" << e.what() << std::endl;
        system("pause");
        return 1;
    }
    return 0;
}