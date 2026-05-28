#include <SFML/Graphics.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <cstring>
#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
}

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;
const int SCALE = 3;
const double TARGET_FPS = 24.0;

const uint8_t OSD_FONT[256][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // пробел
    {0x18,0x24,0x42,0x42,0x7E,0x42,0x42,0x00}, // A
    {0x7C,0x42,0x42,0x7C,0x42,0x42,0x7C,0x00}, // B
    {0x3C,0x42,0x40,0x40,0x40,0x42,0x3C,0x00}, // C
    {0x78,0x44,0x42,0x42,0x42,0x44,0x78,0x00}, // D
    {0x7E,0x40,0x40,0x78,0x40,0x40,0x7E,0x00}, // E
    {0x7E,0x40,0x40,0x78,0x40,0x40,0x40,0x00}, // F
    {0x3C,0x42,0x40,0x4C,0x42,0x42,0x3C,0x00}, // G
    {0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x00}, // H
    {0x3E,0x08,0x08,0x08,0x08,0x08,0x3E,0x00}, // I
    {0x1E,0x04,0x04,0x04,0x04,0x44,0x38,0x00}, // J
    {0x42,0x44,0x48,0x50,0x48,0x44,0x42,0x00}, // K
    {0x40,0x40,0x40,0x40,0x40,0x40,0x7E,0x00}, // L
    {0x42,0x66,0x5A,0x42,0x42,0x42,0x42,0x00}, // M
    {0x42,0x62,0x52,0x4A,0x46,0x42,0x42,0x00}, // N
    {0x3C,0x42,0x42,0x42,0x42,0x42,0x3C,0x00}, // O
    {0x7C,0x42,0x42,0x7C,0x40,0x40,0x40,0x00}, // P
    {0x3C,0x42,0x42,0x42,0x4A,0x44,0x3A,0x00}, // Q
    {0x7C,0x42,0x42,0x7C,0x48,0x44,0x42,0x00}, // R
    {0x3C,0x42,0x40,0x3C,0x02,0x42,0x3C,0x00}, // S
    {0x7E,0x08,0x08,0x08,0x08,0x08,0x08,0x00}, // T
    {0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00}, // U
    {0x42,0x42,0x42,0x42,0x24,0x24,0x18,0x00}, // V
    {0x42,0x42,0x42,0x5A,0x5A,0x66,0x42,0x00}, // W
    {0x42,0x42,0x24,0x18,0x24,0x42,0x42,0x00}, // X
    {0x42,0x42,0x24,0x18,0x08,0x08,0x08,0x00}, // Y
    {0x7E,0x02,0x04,0x08,0x10,0x20,0x7E,0x00}, // Z
    {0x3C,0x46,0x4A,0x52,0x62,0x42,0x3C,0x00}, // 0
    {0x18,0x28,0x08,0x08,0x08,0x08,0x3E,0x00}, // 1
    {0x3C,0x42,0x02,0x3C,0x40,0x40,0x7E,0x00}, // 2
    {0x3C,0x42,0x02,0x1C,0x02,0x42,0x3C,0x00}, // 3
    {0x08,0x18,0x28,0x48,0x7E,0x08,0x08,0x00}, // 4
    {0x7E,0x40,0x7C,0x02,0x02,0x42,0x3C,0x00}, // 5
    {0x3C,0x40,0x7C,0x42,0x42,0x42,0x3C,0x00}, // 6
    {0x7E,0x02,0x04,0x08,0x10,0x20,0x20,0x00}, // 7
    {0x3C,0x42,0x42,0x3C,0x42,0x42,0x3C,0x00}, // 8
    {0x3C,0x42,0x42,0x3E,0x02,0x42,0x3C,0x00}, // 9
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // :
    {0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x00}, // -
    {0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00}, // >
    {0x04,0x08,0x10,0x20,0x10,0x08,0x04,0x00}, // <
    {0x02,0x04,0x08,0x10,0x20,0x40,0x00,0x00}, // /
    {0x08,0x10,0x20,0x20,0x20,0x10,0x08,0x00}, // (
    {0x20,0x10,0x08,0x08,0x08,0x10,0x20,0x00}, // )
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // .
    {0x08,0x08,0x08,0x08,0x08,0x00,0x08,0x00}, // !
    {0x3C,0x42,0x02,0x0C,0x10,0x00,0x10,0x00}, // ?
    {0x00,0x18,0x3C,0x7E,0x7E,0x3C,0x18,0x00}  // %
};

int get_font_idx(char c) {
    if (c == ' ') return 0;
    if (c >= 'A' && c <= 'Z') return 1 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 1 + (c - 'a');
    if (c >= '0' && c <= '9') return 27 + (c - '0');
    switch (c) {
        case ':': return 37; case '-': return 38; case '>': return 39;
        case '<': return 40; case '/': return 41; case '(': return 42;
        case ')': return 43; case '.': return 44; case '!': return 45;
        case '?': return 46; case '%': return 47; default: return 0;
    }
}

enum class CpuArch { PULSE, Z80 };

namespace PulseArch {
    enum Opcode : uint8_t {
        NOP = 0x00, MOV = 0x01, ADD = 0x02, LOAD_FRAME = 0x03, JMP = 0x09, HALT = 0x0E, MOVI = 0x21
    };
}

namespace Z80Arch {
    enum Opcode : uint8_t {
        NOP = 0x00, 
        LD_A_N = 0x3E,
        LD_B_N = 0x06,
        ADD_A_B = 0x80,
        JP_NN = 0xC3,  
        HALT = 0x76,   
        RST_18 = 0xDF  
    };
}

class CoreSystem {
public:
    std::mutex vram_mutex;
    CpuArch current_arch;
    
    uint32_t regs[16] = {0}; 
    uint32_t PC = 0;         
    bool running = true;
    bool has_disc = false;

    std::vector<uint8_t> ROM;
    std::vector<uint8_t> VRAM;        
    std::vector<uint8_t> RENDER_VRAM; 

    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    int video_stream_idx = -1;
    double time_base = 0;
    double duration_sec = 0;

    enum PlayerState { STOPPED, DISC_MENU, PLAYING, PAUSED };
    PlayerState state = STOPPED;

    int disc_menu_selected = 0;
    const int DISC_MENU_ITEMS = 2;

    bool show_osd_menu = false;
    float osd_anim = 0.0f; 
    int menu_selected = 0;
    std::vector<std::string> menu_labels = {"speed", "zoom", "osd info", "chapter"};
    int current_speed = 1;
    bool zoom_enabled = false;
    bool info_osd = true;

    bool show_fx_menu = false;
    float fx_anim = 0.0f; 
    int fx_selected = 0;
    std::vector<std::string> fx_labels = {"bright", "contrast", "scanlines", "grayscale"};
    int fx_bright = 100;
    int fx_contrast = 100;
    bool fx_scanlines = false;
    bool fx_grayscale = false;

    int current_chapter = 1;
    int total_chapters = 1;
    const double CHAPTER_LENGTH_SEC = 300.0;

    int logo_x = 100, logo_y = 100, logo_dx = 3, logo_dy = 2, logo_color = 0;

    CoreSystem(std::vector<uint8_t> code, const std::string& cpu_name) {
        ROM = code;
        VRAM.resize(SCREEN_WIDTH * SCREEN_HEIGHT * 3, 0);
        RENDER_VRAM.resize(SCREEN_WIDTH * SCREEN_HEIGHT * 3, 0);
        
        if (cpu_name == "z80") current_arch = CpuArch::Z80;
        else current_arch = CpuArch::PULSE;
        
        av_log_set_level(AV_LOG_FATAL); 
        
        std::cout << "[sys] ядро coreDVD инициализировано." << std::endl;
        std::cout << "[sys] установлен процессор: " << (current_arch == CpuArch::PULSE ? "pulse 32-bit" : "z80 8-bit") << std::endl;
        std::cout << "[sys] ищем медиа данные..." << std::endl;
        
        std::string video_file = find_video();
        if (!video_file.empty() && init_ffmpeg(video_file)) {
            has_disc = true;
            state = DISC_MENU; 
            std::cout << "[media] поток успешно подхвачен: " << video_file << std::endl;
        } else {
            std::cout << "[media] диск не найден. ожидание данных..." << std::endl;
        }
    }

    ~CoreSystem() {
        if (sws_ctx) sws_freeContext(sws_ctx);
        if (codec_ctx) avcodec_free_context(&codec_ctx);
        if (fmt_ctx) avformat_close_input(&fmt_ctx);
        if (frame) av_frame_free(&frame);
        if (pkt) av_packet_free(&pkt);
    }

    std::string find_video() {
        if (std::filesystem::exists("VIDEO_TS/VTS_01_1.VOB")) return "VIDEO_TS/VTS_01_1.VOB";
        if (std::filesystem::exists("movie.mp4")) return "movie.mp4";
        if (std::filesystem::exists("video.mkv")) return "video.mkv";
        return "";
    }

    bool init_ffmpeg(const std::string& filename) {
        fmt_ctx = avformat_alloc_context();
        if (avformat_open_input(&fmt_ctx, filename.c_str(), nullptr, nullptr) != 0) return false;
        
        fmt_ctx->probesize = 100 * 1024 * 1024;
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) return false;

        video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_idx < 0) return false;

        const AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
        if (!codec) return false;

        codec_ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) return false;

        frame = av_frame_alloc();
        pkt = av_packet_alloc();

        sws_ctx = sws_getContext(
            codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
            SCREEN_WIDTH, SCREEN_HEIGHT, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        time_base = av_q2d(fmt_ctx->streams[video_stream_idx]->time_base);
        duration_sec = fmt_ctx->duration / (double)AV_TIME_BASE;
        if (duration_sec <= 0) duration_sec = 600; 
        
        total_chapters = (int)(duration_sec / CHAPTER_LENGTH_SEC) + 1;
        return true;
    }

    void seek_chapter(int chapter) {
        if (!has_disc || chapter < 1) return;
        if (chapter > total_chapters) chapter = total_chapters;
        current_chapter = chapter;
        
        double target_sec = (chapter - 1) * CHAPTER_LENGTH_SEC;
        if (target_sec >= duration_sec) target_sec = std::max(0.0, duration_sec - 1.0);

        int64_t target_pts = target_sec / time_base;
        av_seek_frame(fmt_ctx, video_stream_idx, target_pts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codec_ctx);
        
        regs[15] = target_sec * TARGET_FPS; 
    }

    bool decode_next_frame() {
        if (!has_disc) return false;

        while (av_read_frame(fmt_ctx, pkt) >= 0) {
            if (pkt->stream_index == video_stream_idx) {
                if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                    if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                        uint8_t* dest[4] = { VRAM.data(), nullptr, nullptr, nullptr };
                        int dest_linesize[4] = { SCREEN_WIDTH * 3, 0, 0, 0 };
                        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, dest, dest_linesize);
                        av_packet_unref(pkt);
                        return true; 
                    }
                }
            }
            av_packet_unref(pkt);
        }
        return false; 
    }

    void apply_video_effects() {
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int x = 0; x < SCREEN_WIDTH; ++x) {
                int src_x = zoom_enabled ? (SCREEN_WIDTH/4 + x/2) : x;
                int src_y = zoom_enabled ? (SCREEN_HEIGHT/4 + y/2) : y;
                
                int src_off = (src_y * SCREEN_WIDTH + src_x) * 3;
                int dst_off = (y * SCREEN_WIDTH + x) * 3;

                int r = VRAM[src_off];
                int g = VRAM[src_off+1];
                int b = VRAM[src_off+2];

                if (fx_grayscale) {
                    int gray = (r * 77 + g * 150 + b * 29) >> 8;
                    r = g = b = gray;
                }
                
                if (fx_bright != 100 || fx_contrast != 100) {
                    r = std::clamp(((r - 128) * fx_contrast / 100) + 128 + (fx_bright - 100), 0, 255);
                    g = std::clamp(((g - 128) * fx_contrast / 100) + 128 + (fx_bright - 100), 0, 255);
                    b = std::clamp(((b - 128) * fx_contrast / 100) + 128 + (fx_bright - 100), 0, 255);
                }

                if (fx_scanlines && (y % 2 == 0)) {
                    r = (r * 3) >> 2;
                    g = (g * 3) >> 2;
                    b = (b * 3) >> 2;
                }

                RENDER_VRAM[dst_off]   = r;
                RENDER_VRAM[dst_off+1] = g;
                RENDER_VRAM[dst_off+2] = b;
            }
        }
    }

    void clear_render() { std::fill(RENDER_VRAM.begin(), RENDER_VRAM.end(), 0); }

    void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            int off = (y * SCREEN_WIDTH + x) * 3;
            RENDER_VRAM[off] = r; RENDER_VRAM[off+1] = g; RENDER_VRAM[off+2] = b;
        }
    }

    void draw_text(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        for (size_t i = 0; i < text.size(); ++i) {
            int idx = get_font_idx(text[i]);
            for (int row = 0; row < 8; ++row) {
                uint8_t bits = OSD_FONT[idx][row];
                for (int col = 0; col < 8; ++col) {
                    if (bits & (0x80 >> col)) put_pixel(x + i * 8 + col, y + row, r, g, b);
                }
            }
        }
    }

    void draw_text_shadow(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        draw_text(text, x + 1, y + 1, 0, 0, 0); 
        draw_text(text, x, y, r, g, b);         
    }

    void draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        for (int j = y; j < y + h; ++j) {
            for (int i = x; i < x + w; ++i) put_pixel(i, j, r, g, b);
        }
    }

    void render_disc_menu() {
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            uint8_t blue = 100 + (y * 155 / SCREEN_HEIGHT);
            draw_rect(0, y, SCREEN_WIDTH, 1, 10, 20, blue);
        }
        draw_text_shadow("coreDVD os", 100, 40, 255, 200, 50);
        
        std::vector<std::string> disc_items = {"play media", "scene selection"};
        for (int i = 0; i < DISC_MENU_ITEMS; ++i) {
            int y_pos = 100 + i * 30;
            if (i == disc_menu_selected) {
                draw_rect(80, y_pos - 4, 160, 16, 200, 50, 50); 
                draw_text_shadow(disc_items[i], 90, y_pos, 255, 255, 255);
            } else {
                draw_text_shadow(disc_items[i], 90, y_pos, 150, 150, 150);
            }
        }
    }

    void render_osd_menu() {
        if (osd_anim < 1.0f) osd_anim += 0.15f;
        if (osd_anim > 1.0f) osd_anim = 1.0f;

        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 3; i++) RENDER_VRAM[i] = RENDER_VRAM[i] * 3 / 4; 
        
        int menu_w = 244;
        int menu_h = 130;
        int mx = (SCREEN_WIDTH - menu_w) / 2;
        int target_my = (SCREEN_HEIGHT - menu_h) / 2;
        int my = target_my - (int)((1.0f - osd_anim) * 200);

        draw_rect(mx - 2, my - 2, menu_w + 4, menu_h + 4, 200, 200, 200); 
        draw_rect(mx, my, menu_w, menu_h, 30, 40, 60);                    
        
        draw_text_shadow("core setup", mx + 80, my + 15, 255, 255, 255);
        draw_rect(mx + 10, my + 30, menu_w - 20, 2, 100, 110, 130);   

        for (int i = 0; i < menu_labels.size(); ++i) {
            std::string val = "";
            switch (i) {
                case 0: val = std::to_string(current_speed) + "x"; break;
                case 1: val = zoom_enabled ? "on" : "off"; break;
                case 2: val = info_osd ? "on" : "off"; break;
                case 3: val = std::to_string(current_chapter) + "/" + std::to_string(total_chapters); break;
            }
            
            int text_y = my + 45 + i * 18;
            if (i == menu_selected) {
                draw_rect(mx + 10, text_y - 2, menu_w - 20, 12, 70, 130, 200);
                draw_text_shadow(menu_labels[i], mx + 20, text_y, 255, 255, 255);
                draw_text_shadow(val, mx + 180, text_y, 255, 255, 0);
            } else {
                draw_text_shadow(menu_labels[i], mx + 20, text_y, 180, 180, 180);
                draw_text_shadow(val, mx + 180, text_y, 150, 150, 150);
            }
        }
    }

    void render_fx_menu() {
        if (fx_anim < 1.0f) fx_anim += 0.15f;
        if (fx_anim > 1.0f) fx_anim = 1.0f;

        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 3; i++) RENDER_VRAM[i] = RENDER_VRAM[i] * 4 / 5;

        int menu_w = 200;
        int menu_h = 120;
        int target_mx = 20;
        int mx = target_mx - (int)((1.0f - fx_anim) * 250); 
        int my = 20;

        draw_rect(mx - 2, my - 2, menu_w + 4, menu_h + 4, 255, 100, 100); 
        draw_rect(mx, my, menu_w, menu_h, 60, 20, 20); 

        draw_text_shadow("video fx core", mx + 45, my + 10, 255, 200, 200);
        draw_rect(mx + 10, my + 25, menu_w - 20, 2, 150, 50, 50);

        for (int i = 0; i < fx_labels.size(); ++i) {
            std::string val = "";
            switch (i) {
                case 0: val = std::to_string(fx_bright) + "%"; break;
                case 1: val = std::to_string(fx_contrast) + "%"; break;
                case 2: val = fx_scanlines ? "on" : "off"; break;
                case 3: val = fx_grayscale ? "on" : "off"; break;
            }

            int text_y = my + 35 + i * 18;
            if (i == fx_selected) {
                draw_rect(mx + 10, text_y - 2, menu_w - 20, 12, 150, 40, 40);
                draw_text_shadow(fx_labels[i], mx + 15, text_y, 255, 255, 255);
                draw_text_shadow(val, mx + 140, text_y, 255, 255, 0);
            } else {
                draw_text_shadow(fx_labels[i], mx + 15, text_y, 200, 150, 150);
                draw_text_shadow(val, mx + 140, text_y, 200, 150, 150);
            }
        }
    }

    void draw_playback_osd() {
        apply_video_effects();

        if (!info_osd) return;

        int frame_number = regs[15];
        int sec = frame_number / TARGET_FPS;
        
        current_chapter = (sec / CHAPTER_LENGTH_SEC) + 1;
        if (current_chapter > total_chapters) current_chapter = total_chapters;

        int min = sec / 60;
        int hrs = min / 60;
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hrs % 24, min % 60, sec % 60);
        
        if (state == PLAYING) draw_text_shadow("play", 15, 15, 0, 255, 0);
        else if (state == PAUSED) draw_text_shadow("pause", 15, 15, 255, 255, 0);
        
        draw_text_shadow(buf, SCREEN_WIDTH - 85, 15, 0, 255, 0);

        int bar_width = (sec * (SCREEN_WIDTH - 20)) / (duration_sec > 0 ? duration_sec : 1);
        draw_rect(10, SCREEN_HEIGHT - 12, SCREEN_WIDTH - 20, 8, 50, 50, 50); 
        draw_rect(10, SCREEN_HEIGHT - 12, bar_width, 8, 0, 200, 50);         
    }

    void step() {
        if (!running) return;

        if (!has_disc) {
            std::lock_guard<std::mutex> lock(vram_mutex);
            animate_logo();
            return;
        }

        if (show_osd_menu) {
            std::lock_guard<std::mutex> lock(vram_mutex);
            render_osd_menu();
            return;
        }
        
        if (show_fx_menu) {
            std::lock_guard<std::mutex> lock(vram_mutex);
            render_fx_menu();
            return;
        }

        if (state == DISC_MENU) {
            std::lock_guard<std::mutex> lock(vram_mutex);
            render_disc_menu();
            return;
        }

        if (PC >= ROM.size()) PC = 0;

        if (current_arch == CpuArch::PULSE) {
            step_pulse();
        } else if (current_arch == CpuArch::Z80) {
            step_z80();
        }
    }

    void step_pulse() {
        uint8_t opcode = ROM[PC++];

        switch (opcode) {
        case PulseArch::NOP: break;
        case PulseArch::MOV: {
            uint8_t rd = ROM[PC++];
            regs[rd] = ROM[PC] | (ROM[PC+1]<<8) | (ROM[PC+2]<<16) | (ROM[PC+3]<<24);
            PC += 4;
            break;
        }
        case PulseArch::MOVI: { 
            uint8_t rd = ROM[PC++];
            regs[rd] = ROM[PC] | (ROM[PC+1] << 8);
            PC += 2; break;
        }
        case PulseArch::ADD: {
            uint8_t rd = ROM[PC++];
            uint8_t rs = ROM[PC++];
            regs[rd] += regs[rs]; 
            break;
        }
        case PulseArch::JMP: {
            PC = ROM[PC] | (ROM[PC+1]<<8) | (ROM[PC+2]<<16) | (ROM[PC+3]<<24);
            break;
        }
        case PulseArch::LOAD_FRAME: {
            std::lock_guard<std::mutex> lock(vram_mutex);
            if (state == PLAYING) {
                for (int i=0; i < current_speed; i++) {
                    if (!decode_next_frame()) {
                        state = DISC_MENU; 
                        clear_render();
                        break;
                    }
                    regs[15]++; 
                }
            }
            if (state == PLAYING || state == PAUSED) {
                draw_playback_osd();
            }
            PC -= 1; 
            break;
        }
        case PulseArch::HALT: running = false; break;
        default: break;
        }
    }

    void step_z80() {
        uint8_t opcode = ROM[PC++];
        
        switch (opcode) {
        case Z80Arch::NOP: break;
        case Z80Arch::LD_A_N: {
            regs[0] = ROM[PC++]; 
            break;
        }
        case Z80Arch::LD_B_N: {
            regs[1] = ROM[PC++]; 
            break;
        }
        case Z80Arch::ADD_A_B: {
            regs[0] = (regs[0] + regs[1]) & 0xFF; 
            break;
        }
        case Z80Arch::JP_NN: {
            PC = ROM[PC] | (ROM[PC+1] << 8); 
            break;
        }
        case Z80Arch::RST_18: { 
            std::lock_guard<std::mutex> lock(vram_mutex);
            if (state == PLAYING) {
                for (int i=0; i < current_speed; i++) {
                    if (!decode_next_frame()) {
                        state = DISC_MENU; 
                        clear_render();
                        break;
                    }
                    regs[15]++; 
                }
            }
            if (state == PLAYING || state == PAUSED) draw_playback_osd();
            PC -= 1; 
            break;
        }
        case Z80Arch::HALT: running = false; break;
        default: break;
        }
    }

    void animate_logo() {
        clear_render();
        logo_x += logo_dx; logo_y += logo_dy;
        int w = 56, h = 12; 
        if (logo_x <= 0 || logo_x + w >= SCREEN_WIDTH) { logo_dx = -logo_dx; logo_color = (logo_color + 1) % 4; }
        if (logo_y <= 0 || logo_y + h >= SCREEN_HEIGHT) { logo_dy = -logo_dy; logo_color = (logo_color + 1) % 4; }
        uint8_t cr=255, cg=255, cb=255;
        switch (logo_color) {
            case 1: cr=255; cg=255; cb=0; break;
            case 2: cr=0; cg=255; cb=255; break;
            case 3: cr=255; cg=0; cb=255; break;
        }
        
        std::string cpu_str = current_arch == CpuArch::PULSE ? "pulse" : "z80";
        draw_text_shadow("coreDVD " + cpu_str, logo_x, logo_y, cr, cg, cb);
    }
};

void cpu_thread_func(CoreSystem& cpu, double fps) {
    using namespace std::chrono;
    auto frame_duration = duration_cast<steady_clock::duration>(duration<double>(1.0 / fps));
    auto next_frame = steady_clock::now();

    while (cpu.running) {
        cpu.step();
        auto now = steady_clock::now();
        if (now < next_frame) std::this_thread::sleep_until(next_frame);
        else next_frame = now;
        next_frame += frame_duration;
    }
}

std::vector<uint8_t> load_firmware(const std::string& filename, const std::string& cpu_name) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "[sys] прошивка " << filename << " не найдена, используем дефолт." << std::endl;
        if (cpu_name == "z80") return {0xDF, 0x76}; 
        return {0x03, 0x0E}; 
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    std::cout << "[sys] загружена прошивка (" << size << " байт)" << std::endl;
    return buffer;
}

int main(int argc, char* argv[]) {
    std::string cpu_name = "pulse";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--cpu=") == 0) {
            cpu_name = arg.substr(6);
        }
    }

    std::cout << "\n======================================\n";
    std::cout << "        project coreDVD boot          \n";
    std::cout << "        cpu: " << cpu_name << "       \n";
    std::cout << "======================================\n\n";

    std::string fw_path = "firmware/" + cpu_name + "/firmware.bin";
    std::vector<uint8_t> firmware = load_firmware(fw_path, cpu_name);

    CoreSystem cpu(firmware, cpu_name);
    std::thread cpu_thread(cpu_thread_func, std::ref(cpu), TARGET_FPS);

    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE),
                            "project coreDVD (" + cpu_name + ")", sf::Style::Close);
    window.setFramerateLimit(60); 

    sf::Texture texture;
    texture.create(SCREEN_WIDTH, SCREEN_HEIGHT);
    sf::Sprite sprite(texture);
    sprite.setScale(SCALE, SCALE);
    std::vector<sf::Uint8> pixels(SCREEN_WIDTH * SCREEN_HEIGHT * 4, 255);

    std::cout << "\n--- хоткеи системы ---\n";
    std::cout << "[m]    : настройки ядра (скорость, зум)\n";
    std::cout << "[v]    : видео fx (яркость, контраст, сканлайны)\n";
    std::cout << "[esc]  : закрыть меню\n";
    std::cout << "[space]: плей / пауза\n";
    std::cout << "[s]    : выход в os coreDVD (меню диска)\n";

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                cpu.running = false;
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                std::lock_guard<std::mutex> lock(cpu.vram_mutex); 
                
                if (event.key.code == sf::Keyboard::M) {
                    if (!cpu.show_osd_menu && cpu.has_disc) {
                        cpu.show_osd_menu = true;
                        cpu.show_fx_menu = false; 
                        cpu.osd_anim = 0.0f; 
                    }
                }
                else if (event.key.code == sf::Keyboard::V) {
                    if (!cpu.show_fx_menu && cpu.has_disc) {
                        cpu.show_fx_menu = true;
                        cpu.show_osd_menu = false;
                        cpu.fx_anim = 0.0f; 
                    }
                }
                else if (event.key.code == sf::Keyboard::Escape) {
                    if (cpu.show_osd_menu || cpu.show_fx_menu) {
                        cpu.show_osd_menu = false;
                        cpu.show_fx_menu = false;
                        if (cpu.state == CoreSystem::PAUSED) cpu.draw_playback_osd();
                    }
                }
                else if (cpu.show_osd_menu) { 
                    if (event.key.code == sf::Keyboard::Down) cpu.menu_selected = (cpu.menu_selected + 1) % cpu.menu_labels.size();
                    if (event.key.code == sf::Keyboard::Up) cpu.menu_selected = (cpu.menu_selected - 1 + cpu.menu_labels.size()) % cpu.menu_labels.size();
                    
                    if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::Left) {
                        int dir = (event.key.code == sf::Keyboard::Right) ? 1 : -1;
                        switch (cpu.menu_selected) {
                            case 0:
                                if (dir == 1 && cpu.current_speed < 4) cpu.current_speed *= 2;
                                if (dir == -1 && cpu.current_speed > 1) cpu.current_speed /= 2;
                                break;
                            case 1: cpu.zoom_enabled = !cpu.zoom_enabled; break;
                            case 2: cpu.info_osd = !cpu.info_osd; break;
                            case 3:
                                if (dir == 1) cpu.seek_chapter(cpu.current_chapter + 1);
                                if (dir == -1) cpu.seek_chapter(cpu.current_chapter - 1);
                                break;
                        }
                    }
                }
                else if (cpu.show_fx_menu) {
                    if (event.key.code == sf::Keyboard::Down) cpu.fx_selected = (cpu.fx_selected + 1) % cpu.fx_labels.size();
                    if (event.key.code == sf::Keyboard::Up) cpu.fx_selected = (cpu.fx_selected - 1 + cpu.fx_labels.size()) % cpu.fx_labels.size();

                    if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::Left) {
                        int dir = (event.key.code == sf::Keyboard::Right) ? 10 : -10;
                        switch (cpu.fx_selected) {
                            case 0: cpu.fx_bright = std::clamp(cpu.fx_bright + dir, 0, 200); cpu.draw_playback_osd(); break;
                            case 1: cpu.fx_contrast = std::clamp(cpu.fx_contrast + dir, 0, 200); cpu.draw_playback_osd(); break;
                            case 2: cpu.fx_scanlines = !cpu.fx_scanlines; cpu.draw_playback_osd(); break;
                            case 3: cpu.fx_grayscale = !cpu.fx_grayscale; cpu.draw_playback_osd(); break;
                        }
                    }
                }
                else if (cpu.state == CoreSystem::DISC_MENU) {
                    if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::Up) {
                        cpu.disc_menu_selected = (cpu.disc_menu_selected + 1) % cpu.DISC_MENU_ITEMS;
                    }
                    if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                        cpu.state = CoreSystem::PLAYING;
                        if (cpu.disc_menu_selected == 0) cpu.seek_chapter(1);
                        else cpu.seek_chapter(2); 
                    }
                }
                else { 
                    if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::Enter) {
                        if (cpu.state == CoreSystem::PLAYING) cpu.state = CoreSystem::PAUSED;
                        else if (cpu.state == CoreSystem::PAUSED) cpu.state = CoreSystem::PLAYING;
                    }
                    if (event.key.code == sf::Keyboard::S) {
                        cpu.state = CoreSystem::DISC_MENU;
                        cpu.clear_render();
                    }
                    if (event.key.code == sf::Keyboard::Right) cpu.seek_chapter(cpu.current_chapter + 1);
                    if (event.key.code == sf::Keyboard::Left) cpu.seek_chapter(cpu.current_chapter - 1);
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(cpu.vram_mutex);
            for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
                pixels[i*4]   = cpu.RENDER_VRAM[i*3];
                pixels[i*4+1] = cpu.RENDER_VRAM[i*3+1];
                pixels[i*4+2] = cpu.RENDER_VRAM[i*3+2];
                pixels[i*4+3] = 255;
            }
        }

        texture.update(pixels.data());
        window.clear();
        window.draw(sprite);
        window.display();
    }

    if (cpu_thread.joinable()) cpu_thread.join();
    return 0;
}
