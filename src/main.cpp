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

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;
const int SCALE = 3;
const double TARGET_FPS = 24.0;   // частота кадров DVD

// аппаратный шрифт 8×8
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
    {0x3C,0x42,0x02,0x0C,0x10,0x00,0x10,0x00}  // ?
};

int get_font_idx(char c) {
    if (c == ' ') return 0;
    if (c >= 'A' && c <= 'Z') return 1 + (c - 'A');
    if (c >= '0' && c <= '9') return 27 + (c - '0');
    switch (c) {
        case ':': return 37;
        case '-': return 38;
        case '>': return 39;
        case '<': return 40;
        case '/': return 41;
        case '(': return 42;
        case ')': return 43;
        case '.': return 44;
        case '!': return 45;
        case '?': return 46;
        default: return 0;
    }
}

// Расширенный набор инструкций
enum Opcode : uint8_t {
    NOP        = 0x00,
    MOV        = 0x01,
    ADD        = 0x02,
    LOAD_FRAME = 0x03,
    SUB        = 0x04,
    AND        = 0x05,
    OR         = 0x06,
    XOR        = 0x07,
    CMP        = 0x08,
    JMP        = 0x09,
    JZ         = 0x0A,
    JNZ        = 0x0B,
    LOAD       = 0x0C,
    STORE      = 0x0D,
    HALT       = 0x0E,
    INC        = 0x0F,
    DEC        = 0x10,
    SHL        = 0x11,
    SHR        = 0x12,
    NOT        = 0x13,
    NEG        = 0x14,
    CALL       = 0x15,
    RET        = 0x16,
    PRINT_CHAR = 0x17,
    // новые инструкции
    MUL        = 0x18,
    DIV        = 0x19,
    MOD        = 0x1A,
    LSL        = 0x1B,
    LSR        = 0x1C,
    ASR        = 0x1D,
    ROR        = 0x1E,
    PUSH       = 0x1F,
    POP        = 0x20,
    MOVI       = 0x21,
    LDRB       = 0x22,
    STRB       = 0x23,
    LDRH       = 0x24,
    STRH       = 0x25,
    SWI        = 0x26
};

const uint32_t FLAG_Z = 1 << 0;

class AdvancedDVDProcessor {
public:
    std::mutex vram_mutex;
    uint32_t regs[16] = {0};
    uint32_t PC = 0;
    uint32_t flags = 0;
    bool running = true;
    bool has_disc = false;

    std::vector<uint8_t> ROM;
    std::vector<uint8_t> RAM;   // 64 КБ ОЗУ
    std::vector<uint8_t> VRAM;
    std::vector<uint8_t> VOB_DATA;

    // таблицы YUV → RGB
    int Y_base[256];
    int R_V_tab[256];
    int G_U_tab[256];
    int G_V_tab[256];
    int B_U_tab[256];

    enum PlayerState { STOPPED, PLAYING, PAUSED, MENU };
    PlayerState state = STOPPED;

    bool show_osd_menu = false;
    int menu_selected = 0;
    const int MENU_ITEMS = 6;
    std::vector<std::string> menu_labels = {
        "SPEED", "ZOOM", "OSD INFO", "AUDIO TR", "SUBTITLE", "CHAPTER"
    };
    int current_speed = 1;
    bool zoom_enabled = false;
    bool info_osd = true;
    int audio_track = 1;
    bool subtitle_on = false;
    int current_chapter = 1;
    int total_chapters = 5;
    int chapter_start_frame[5] = {0, 300, 600, 900, 1200};

    struct Subtitle {
        int start_frame;
        int end_frame;
        std::string text;
    };
    std::vector<Subtitle> subtitles = {
        {0, 100, "HELLO WORLD!"},
        {150, 300, "DVD PLAYER"},
        {400, 600, "PERFECTION"},
        {700, 900, "ENJOY"}
    };

    int logo_x = 100, logo_y = 100;
    int logo_dx = 3, logo_dy = 2;
    int logo_color = 0;

    // Отладка
    bool debug_enabled = false;
    int debug_step_counter = 0;
    void debug_print_state(uint8_t opcode);

    AdvancedDVDProcessor(std::vector<uint8_t> code) {
        ROM = code;
        RAM.resize(65536, 0);
        VRAM.resize(SCREEN_WIDTH * SCREEN_HEIGHT * 3, 0);
        regs[14] = 0xFFFF; // стек растёт вниз от верха 64 КБ
        build_yuv_tables();
        load_vob_or_generate();
        std::cout << "[SYS] dvd processor started, rom: " << ROM.size() << " bytes, ram: 64kb" << std::endl;
    }

    void build_yuv_tables() {
        for (int i = 0; i < 256; ++i) {
            int c = i - 16;
            int d = i - 128;
            Y_base[i] = (298 * c) >> 8;
            R_V_tab[i] = (409 * d) >> 8;
            G_U_tab[i] = (-100 * d) >> 8;
            G_V_tab[i] = (-208 * d) >> 8;
            B_U_tab[i] = (516 * d) >> 8;
        }
    }

    void load_vob_or_generate() {
        std::ifstream file("VIDEO_TS/VTS_01_1.VOB", std::ios::binary);
        if (file.is_open()) {
            VOB_DATA = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                            std::istreambuf_iterator<char>());
            has_disc = true;
            std::cout << "[SYS] loaded VOB file, size: " << VOB_DATA.size() << " bytes" << std::endl;
        } else {
            has_disc = true;
            int frame_size = SCREEN_WIDTH * SCREEN_HEIGHT * 3 / 2;
            int total_frames = 1500;
            VOB_DATA.resize(frame_size * total_frames);
            for (int f = 0; f < total_frames; ++f) {
                uint8_t* Y = &VOB_DATA[f * frame_size];
                uint8_t* U = Y + SCREEN_WIDTH * SCREEN_HEIGHT;
                uint8_t* V = U + SCREEN_WIDTH * SCREEN_HEIGHT / 4;
                for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                    for (int x = 0; x < SCREEN_WIDTH; ++x) {
                        int stripe = (x / 40) % 8;
                        Y[y * SCREEN_WIDTH + x] = 16 + stripe * 28;
                    }
                }
                std::fill(U, U + SCREEN_WIDTH * SCREEN_HEIGHT / 4, 128);
                std::fill(V, V + SCREEN_WIDTH * SCREEN_HEIGHT / 4, 128);
            }
            std::cout << "[SYS] generated test video, frames: " << total_frames << std::endl;
        }
    }

    void clear_screen() {
        std::fill(VRAM.begin(), VRAM.end(), 0);
    }

    void render_current_frame() {
        if (state != PLAYING && state != PAUSED) return;
        uint32_t vob_ptr = regs[0];
        uint32_t frame_size = SCREEN_WIDTH * SCREEN_HEIGHT * 3 / 2;
        if (vob_ptr + frame_size > VOB_DATA.size()) return;

        const uint8_t* Y_plane = &VOB_DATA[vob_ptr];
        const uint8_t* U_plane = Y_plane + SCREEN_WIDTH * SCREEN_HEIGHT;
        const uint8_t* V_plane = U_plane + SCREEN_WIDTH * SCREEN_HEIGHT / 4;

        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int x = 0; x < SCREEN_WIDTH; ++x) {
                int sx = x, sy = y;
                if (zoom_enabled) {
                    sx = SCREEN_WIDTH/4 + x/2;
                    sy = SCREEN_HEIGHT/4 + y/2;
                }
                int y_idx = sy * SCREEN_WIDTH + sx;
                int uv_idx = (sy/2) * (SCREEN_WIDTH/2) + (sx/2);

                int Y = Y_plane[y_idx];
                int U = U_plane[uv_idx];
                int V = V_plane[uv_idx];

                int yb = Y_base[Y];
                int r = std::max(0, std::min(255, yb + R_V_tab[V]));
                int g = std::max(0, std::min(255, yb + G_U_tab[U] + G_V_tab[V]));
                int b = std::max(0, std::min(255, yb + B_U_tab[U]));

                int off = (y * SCREEN_WIDTH + x) * 3;
                VRAM[off]   = (uint8_t)r;
                VRAM[off+1] = (uint8_t)g;
                VRAM[off+2] = (uint8_t)b;
            }
        }
        draw_playback_osd(regs[4]);
    }

    void draw_text(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        for (size_t i = 0; i < text.size(); ++i) {
            int idx = get_font_idx(text[i]);
            for (int row = 0; row < 8; ++row) {
                uint8_t bits = OSD_FONT[idx][row];
                for (int col = 0; col < 8; ++col) {
                    if (bits & (0x80 >> col)) {
                        int px = x + i * 8 + col;
                        int py = y + row;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            int off = (py * SCREEN_WIDTH + px) * 3;
                            VRAM[off] = r; VRAM[off+1] = g; VRAM[off+2] = b;
                        }
                    }
                }
            }
        }
    }

    void draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        for (int j = y; j < y + h; ++j) {
            for (int i = x; i < x + w; ++i) {
                if (i >= 0 && i < SCREEN_WIDTH && j >= 0 && j < SCREEN_HEIGHT) {
                    int off = (j * SCREEN_WIDTH + i) * 3;
                    VRAM[off] = r; VRAM[off+1] = g; VRAM[off+2] = b;
                }
            }
        }
    }

    void render_osd_menu() {
        for (int y = 40; y < 200; ++y) {
            for (int x = 40; x < 280; ++x) {
                int off = (y * SCREEN_WIDTH + x) * 3;
                VRAM[off] /= 4; VRAM[off+1] /= 4; VRAM[off+2] /= 2;
            }
        }
        draw_rect(40, 40, 240, 160, 50, 50, 80);
        draw_text("SETUP MENU", 110, 48, 255, 255, 255);

        for (int i = 0; i < MENU_ITEMS; ++i) {
            std::string line = (i == menu_selected ? "> " : "  ") + menu_labels[i] + ": ";
            switch (i) {
                case 0: line += std::to_string(current_speed) + "X"; break;
                case 1: line += zoom_enabled ? "ON" : "OFF"; break;
                case 2: line += info_osd ? "ON" : "OFF"; break;
                case 3: line += "TRACK " + std::to_string(audio_track); break;
                case 4: line += subtitle_on ? "ON" : "OFF"; break;
                case 5: line += std::to_string(current_chapter) + "/" + std::to_string(total_chapters); break;
            }
            draw_text(line, 60, 75 + i * 18, 255, 255, 0);
        }
        draw_text("ESC: close menu", 60, 195, 200, 200, 200);
    }

    void draw_playback_osd(int frame_number) {
        if (!info_osd) return;
        int sec = frame_number / 24;
        int min = sec / 60;
        int hrs = min / 60;
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hrs % 24, min % 60, sec % 60);
        
        if (state == PLAYING) draw_text("PLAY", 15, 15, 0, 255, 0);
        else if (state == PAUSED) draw_text("PAUSE", 15, 15, 255, 255, 0);
        else draw_text("STOP", 15, 15, 255, 0, 0);
        
        draw_text(buf, SCREEN_WIDTH - 85, 15, 0, 255, 0);

        int chap_start = chapter_start_frame[current_chapter - 1];
        int chap_end = (current_chapter < total_chapters) ? chapter_start_frame[current_chapter] : 1500;
        int chap_len = chap_end - chap_start;
        int pos_in_chap = frame_number - chap_start;
        if (pos_in_chap < 0) pos_in_chap = 0;
        if (pos_in_chap > chap_len) pos_in_chap = chap_len;
        int bar_width = (pos_in_chap * (SCREEN_WIDTH - 20)) / (chap_len ? chap_len : 1);
        draw_rect(10, SCREEN_HEIGHT - 12, SCREEN_WIDTH - 20, 8, 80, 80, 80);
        draw_rect(10, SCREEN_HEIGHT - 12, bar_width, 8, 0, 255, 0);

        if (subtitle_on) {
            for (auto& sub : subtitles) {
                if (frame_number >= sub.start_frame && frame_number <= sub.end_frame) {
                    int text_w = sub.text.size() * 8;
                    int sub_x = (SCREEN_WIDTH - text_w) / 2;
                    draw_rect(sub_x - 4, SCREEN_HEIGHT - 50, text_w + 8, 12, 0, 0, 0);
                    draw_text(sub.text, sub_x, SCREEN_HEIGHT - 50, 255, 255, 255);
                    break;
                }
            }
        }

        if (state == PLAYING) {
            int vol = 80 + (rand() % 40);
            draw_text("AUDIO", 10, SCREEN_HEIGHT - 30, 200, 200, 200);
            draw_rect(60, SCREEN_HEIGHT - 28, vol, 6, 0, 200, 0);
        }
    }

    // вспомогательные методы для чтения из ROM
    uint32_t read_imm32() {
        if (PC + 4 > ROM.size()) { running = false; return 0; }
        uint32_t val = (ROM[PC]<<24)|(ROM[PC+1]<<16)|(ROM[PC+2]<<8)|ROM[PC+3];
        PC += 4;
        return val;
    }

    uint8_t read_reg() {
        if (PC >= ROM.size()) { running = false; return 0; }
        return ROM[PC++];
    }

    void push(uint32_t val) {
        regs[14] -= 4;
        uint32_t addr = regs[14];
        if (addr + 4 <= RAM.size()) {
            RAM[addr]   = val & 0xFF;
            RAM[addr+1] = (val >> 8) & 0xFF;
            RAM[addr+2] = (val >> 16) & 0xFF;
            RAM[addr+3] = (val >> 24) & 0xFF;
        }
    }

    uint32_t pop() {
        uint32_t addr = regs[14];
        regs[14] += 4;
        if (addr + 4 <= RAM.size()) {
            return (uint32_t)RAM[addr] | ((uint32_t)RAM[addr+1]<<8) |
                   ((uint32_t)RAM[addr+2]<<16) | ((uint32_t)RAM[addr+3]<<24);
        }
        return 0;
    }

    void step() {
        if (!running) return;

        if (!has_disc) {
            animate_logo();
            return;
        }

        if (show_osd_menu) {
            render_osd_menu();
            return;
        }

        if (PC >= ROM.size()) PC = 0;
        uint8_t opcode = ROM[PC++];
        debug_print_state(opcode);

        switch (opcode) {
        case NOP: break;
        case MOV: {
            uint8_t rd = read_reg();
            uint32_t imm = read_imm32();
            regs[rd] = imm;
            break;
        }
        case MOVI: { // MOVI rd, imm16 (немедленное 16-битное)
            uint8_t rd = read_reg();
            uint16_t imm = (ROM[PC] << 8) | ROM[PC+1];
            PC += 2;
            regs[rd] = imm;
            break;
        }
        case ADD: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] += regs[rs];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case SUB: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] -= regs[rs];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case MUL: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] *= regs[rs];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case DIV: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            if (regs[rs] != 0) regs[rd] /= regs[rs];
            else regs[rd] = 0;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case MOD: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            if (regs[rs] != 0) regs[rd] %= regs[rs];
            else regs[rd] = 0;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case AND: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] &= regs[rs];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case OR: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] |= regs[rs];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case XOR: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] ^= regs[rs];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case NOT: {
            uint8_t rd = read_reg();
            regs[rd] = ~regs[rd];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case NEG: {
            uint8_t rd = read_reg();
            regs[rd] = -regs[rd];
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case SHL: // устаревший, оставлен для совместимости
        case LSL: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] <<= (regs[rs] & 0x1F);
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case SHR:
        case LSR: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            regs[rd] >>= (regs[rs] & 0x1F);
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case ASR: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            int32_t val = (int32_t)regs[rd];
            val >>= (regs[rs] & 0x1F);
            regs[rd] = (uint32_t)val;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case ROR: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            uint32_t shift = regs[rs] & 0x1F;
            regs[rd] = (regs[rd] >> shift) | (regs[rd] << (32 - shift));
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case CMP: {
            uint8_t rd = read_reg();
            uint8_t rs = read_reg();
            uint32_t res = regs[rd] - regs[rs];
            flags = (res == 0) ? FLAG_Z : 0;
            break;
        }
        case JMP: {
            uint32_t target = read_imm32();
            PC = target;
            break;
        }
        case JZ: {
            uint32_t target = read_imm32();
            if (flags & FLAG_Z) PC = target;
            break;
        }
        case JNZ: {
            uint32_t target = read_imm32();
            if (!(flags & FLAG_Z)) PC = target;
            break;
        }
        case LOAD: {
            uint8_t rd = read_reg();
            uint32_t addr = read_imm32();
            if (addr + 4 <= RAM.size()) {
                regs[rd] = (uint32_t)RAM[addr] | ((uint32_t)RAM[addr+1]<<8) |
                           ((uint32_t)RAM[addr+2]<<16) | ((uint32_t)RAM[addr+3]<<24);
            } else regs[rd] = 0;
            break;
        }
        case STORE: {
            uint32_t addr = read_imm32();
            uint8_t rs = read_reg();
            if (addr + 4 <= RAM.size()) {
                uint32_t val = regs[rs];
                RAM[addr]   = val & 0xFF;
                RAM[addr+1] = (val >> 8) & 0xFF;
                RAM[addr+2] = (val >> 16) & 0xFF;
                RAM[addr+3] = (val >> 24) & 0xFF;
            }
            break;
        }
        case LDRB: {
            uint8_t rd = read_reg();
            uint32_t addr = read_imm32();
            if (addr < RAM.size())
                regs[rd] = RAM[addr];
            else
                regs[rd] = 0;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case STRB: {
            uint32_t addr = read_imm32();
            uint8_t rs = read_reg();
            if (addr < RAM.size())
                RAM[addr] = regs[rs] & 0xFF;
            break;
        }
        case LDRH: {
            uint8_t rd = read_reg();
            uint32_t addr = read_imm32();
            if (addr + 1 < RAM.size()) {
                regs[rd] = (RAM[addr] << 8) | RAM[addr+1];
            } else regs[rd] = 0;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case STRH: {
            uint32_t addr = read_imm32();
            uint8_t rs = read_reg();
            if (addr + 1 < RAM.size()) {
                RAM[addr] = (regs[rs] >> 8) & 0xFF;
                RAM[addr+1] = regs[rs] & 0xFF;
            }
            break;
        }
        case PUSH: {
            uint8_t rs = read_reg();
            push(regs[rs]);
            break;
        }
        case POP: {
            uint8_t rd = read_reg();
            regs[rd] = pop();
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case INC: {
            uint8_t rd = read_reg();
            regs[rd]++;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case DEC: {
            uint8_t rd = read_reg();
            regs[rd]--;
            flags = (regs[rd] == 0) ? FLAG_Z : 0;
            break;
        }
        case CALL: {
            uint32_t target = read_imm32();
            push(PC);
            PC = target;
            break;
        }
        case RET: {
            PC = pop();
            break;
        }
        case PRINT_CHAR: {
            uint8_t rd = read_reg();
            char c = regs[rd] & 0xFF;
            std::cout << c << std::flush;
            break;
        }
        case SWI: {
            uint8_t code = read_reg();
            switch (code) {
                case 0: // системный выход
                    running = false;
                    break;
                case 1: // очистка экрана
                    clear_screen();
                    break;
                default:
                    std::cerr << "[SYS] unknown SWI code: " << (int)code << std::endl;
            }
            break;
        }
        case LOAD_FRAME: {
            std::lock_guard<std::mutex> lock(vram_mutex);
            if (state == PLAYING || state == PAUSED) {
                render_current_frame();
                if (state == PLAYING) {
                    uint32_t frame_size = SCREEN_WIDTH * SCREEN_HEIGHT * 3 / 2;
                    regs[0] += frame_size * current_speed;
                    regs[4] += current_speed;
                }
            } else {
                draw_playback_osd(regs[4]);
            }
            PC = PC - 1; // зацикливаемся (повтор LOAD_FRAME)
            break;
        }
        case HALT:
            running = false;
            break;
        default:
            std::cerr << "[CPU] unknown opcode: 0x" << std::hex << (int)opcode << std::dec << std::endl;
            break;
        }
    }

    void animate_logo() {
        clear_screen();
        logo_x += logo_dx; logo_y += logo_dy;
        int w = 72, h = 12;
        if (logo_x <= 0 || logo_x + w >= SCREEN_WIDTH) {
            logo_dx = -logo_dx;
            logo_color = (logo_color + 1) % 4;
        }
        if (logo_y <= 0 || logo_y + h >= SCREEN_HEIGHT) {
            logo_dy = -logo_dy;
            logo_color = (logo_color + 1) % 4;
        }
        uint8_t cr=255, cg=255, cb=255;
        switch (logo_color) {
            case 1: cr=255; cg=255; cb=0; break;
            case 2: cr=0; cg=255; cb=255; break;
            case 3: cr=255; cg=0; cb=255; break;
        }
        draw_text("DVD-VIDEO", logo_x, logo_y, cr, cg, cb);
    }
};

void AdvancedDVDProcessor::debug_print_state(uint8_t opcode) {
    const int DEBUG_INTERVAL = 1000;
    if (!debug_enabled) return;
    debug_step_counter++;
    if (debug_step_counter % DEBUG_INTERVAL != 0) return;

    std::string op_name;
    switch (opcode) {
        case NOP: op_name = "NOP"; break;
        case MOV: op_name = "MOV"; break;
        case MOVI: op_name = "MOVI"; break;
        case ADD: op_name = "ADD"; break;
        case SUB: op_name = "SUB"; break;
        case MUL: op_name = "MUL"; break;
        case DIV: op_name = "DIV"; break;
        case MOD: op_name = "MOD"; break;
        case AND: op_name = "AND"; break;
        case OR:  op_name = "OR";  break;
        case XOR: op_name = "XOR"; break;
        case NOT: op_name = "NOT"; break;
        case NEG: op_name = "NEG"; break;
        case LSL: op_name = "LSL"; break;
        case LSR: op_name = "LSR"; break;
        case ASR: op_name = "ASR"; break;
        case ROR: op_name = "ROR"; break;
        case CMP: op_name = "CMP"; break;
        case JMP: op_name = "JMP"; break;
        case JZ:  op_name = "JZ";  break;
        case JNZ: op_name = "JNZ"; break;
        case LOAD: op_name = "LOAD"; break;
        case STORE: op_name = "STORE"; break;
        case LDRB: op_name = "LDRB"; break;
        case STRB: op_name = "STRB"; break;
        case LDRH: op_name = "LDRH"; break;
        case STRH: op_name = "STRH"; break;
        case PUSH: op_name = "PUSH"; break;
        case POP: op_name = "POP"; break;
        case INC: op_name = "INC"; break;
        case DEC: op_name = "DEC"; break;
        case CALL: op_name = "CALL"; break;
        case RET: op_name = "RET"; break;
        case PRINT_CHAR: op_name = "PRINT_CHAR"; break;
        case SWI: op_name = "SWI"; break;
        case LOAD_FRAME: op_name = "LOAD_FRAME"; break;
        case HALT: op_name = "HALT"; break;
        default: op_name = "UNKNOWN"; break;
    }

    std::cout << "[DBG] PC=0x" << std::hex << PC - 1 << " [" << op_name << "] regs: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << "R" << i << "=0x" << regs[i] << (i < 15 ? " " : "");
    }
    std::cout << " Z=" << ((flags & FLAG_Z) ? '1' : '0') << std::dec << std::endl;
}

void cpu_thread_func(AdvancedDVDProcessor& cpu, double fps) {
    using namespace std::chrono;
    auto frame_duration = duration_cast<steady_clock::duration>(duration<double>(1.0 / fps));
    auto next_frame = steady_clock::now();

    while (cpu.running) {
        cpu.step();
        auto now = steady_clock::now();
        if (now < next_frame) {
            std::this_thread::sleep_until(next_frame);
        } else {
            next_frame = now;
        }
        next_frame += frame_duration;
    }
}

std::vector<uint8_t> load_firmware(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[SYS] error: cannot open firmware file " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "[SYS] error: failed to read firmware" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "[SYS] firmware loaded, " << size << " bytes" << std::endl;
    return buffer;
}

int main() {
    std::vector<uint8_t> firmware = load_firmware("firmware.bin");

    AdvancedDVDProcessor cpu(firmware);
    std::thread cpu_thread(cpu_thread_func, std::ref(cpu), TARGET_FPS);

    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE),
                            "DVD Player", sf::Style::Close);
    window.setFramerateLimit(0);
    window.setKeyRepeatEnabled(false);

    sf::Texture texture;
    texture.create(SCREEN_WIDTH, SCREEN_HEIGHT);
    sf::Sprite sprite(texture);
    sprite.setScale(SCALE, SCALE);
    std::vector<sf::Uint8> pixels(SCREEN_WIDTH * SCREEN_HEIGHT * 4, 255);

    std::cout << "[SYS] === coreDVD ===\n";
    std::cout << "[SYS] m: open setup menu\n";
    std::cout << "[SYS] esc: close menu\n";
    std::cout << "[SYS] space: play / pause\n";
    std::cout << "[SYS] s: stop\n";
    std::cout << "[SYS] left/right: prev / next chapter\n";
    std::cout << "[SYS] d: toggle debug (quiet mode)\n";
    std::cout << "[SYS] close window to exit.\n";

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                cpu.running = false;
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::M) {
                    if (!cpu.show_osd_menu) {
                        cpu.show_osd_menu = true;
                        std::cout << "[UI] menu opened" << std::endl;
                    }
                }
                else if (event.key.code == sf::Keyboard::Escape) {
                    if (cpu.show_osd_menu) {
                        cpu.show_osd_menu = false;
                        std::cout << "[UI] menu closed" << std::endl;
                        std::lock_guard<std::mutex> lock(cpu.vram_mutex);
                        cpu.clear_screen();
                        cpu.render_current_frame();
                    }
                }
                else if (event.key.code == sf::Keyboard::D) {
                    cpu.debug_enabled = !cpu.debug_enabled;
                    std::cout << "[DEBUG] " << (cpu.debug_enabled ? "enabled" : "disabled") << std::endl;
                }

                if (!cpu.show_osd_menu) {
                    if (event.key.code == sf::Keyboard::Space) {
                        if (cpu.state == AdvancedDVDProcessor::PLAYING) {
                            cpu.state = AdvancedDVDProcessor::PAUSED;
                            std::cout << "[PLAYER] paused" << std::endl;
                        } else if (cpu.state == AdvancedDVDProcessor::PAUSED) {
                            cpu.state = AdvancedDVDProcessor::PLAYING;
                            std::cout << "[PLAYER] resumed" << std::endl;
                        } else if (cpu.state == AdvancedDVDProcessor::STOPPED) {
                            cpu.state = AdvancedDVDProcessor::PLAYING;
                            cpu.regs[0] = 0;
                            cpu.regs[4] = 0;
                            cpu.current_chapter = 1;
                            std::cout << "[PLAYER] playback started" << std::endl;
                        }
                    }
                    if (event.key.code == sf::Keyboard::S) {
                        cpu.state = AdvancedDVDProcessor::STOPPED;
                        cpu.regs[0] = 0;
                        cpu.regs[4] = 0;
                        cpu.current_chapter = 1;
                        cpu.clear_screen();
                        std::cout << "[PLAYER] stopped" << std::endl;
                    }
                    if (event.key.code == sf::Keyboard::Right) {
                        if (cpu.current_chapter < cpu.total_chapters) {
                            cpu.current_chapter++;
                            cpu.regs[0] = cpu.chapter_start_frame[cpu.current_chapter-1] * (SCREEN_WIDTH*SCREEN_HEIGHT*3/2);
                            cpu.regs[4] = cpu.chapter_start_frame[cpu.current_chapter-1];
                            std::cout << "[PLAYER] chapter: " << cpu.current_chapter << "/" << cpu.total_chapters << std::endl;
                        }
                    }
                    if (event.key.code == sf::Keyboard::Left) {
                        if (cpu.current_chapter > 1) {
                            cpu.current_chapter--;
                            cpu.regs[0] = cpu.chapter_start_frame[cpu.current_chapter-1] * (SCREEN_WIDTH*SCREEN_HEIGHT*3/2);
                            cpu.regs[4] = cpu.chapter_start_frame[cpu.current_chapter-1];
                            std::cout << "[PLAYER] chapter: " << cpu.current_chapter << "/" << cpu.total_chapters << std::endl;
                        }
                    }
                } else { // внутри меню
                    if (event.key.code == sf::Keyboard::Down) {
                        cpu.menu_selected = (cpu.menu_selected + 1) % cpu.MENU_ITEMS;
                        std::cout << "[UI] menu selection: " << cpu.menu_labels[cpu.menu_selected] << std::endl;
                    }
                    if (event.key.code == sf::Keyboard::Up) {
                        cpu.menu_selected = (cpu.menu_selected - 1 + cpu.MENU_ITEMS) % cpu.MENU_ITEMS;
                        std::cout << "[UI] menu selection: " << cpu.menu_labels[cpu.menu_selected] << std::endl;
                    }
                    auto& sel = cpu.menu_selected;
                    if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::Left) {
                        int dir = (event.key.code == sf::Keyboard::Right) ? 1 : -1;
                        switch (sel) {
                            case 0:
                                if (dir == 1 && cpu.current_speed < 4) cpu.current_speed *= 2;
                                if (dir == -1 && cpu.current_speed > 1) cpu.current_speed /= 2;
                                std::cout << "[UI] speed set to " << cpu.current_speed << "x" << std::endl;
                                break;
                            case 1:
                                cpu.zoom_enabled = !cpu.zoom_enabled;
                                std::cout << "[UI] zoom: " << (cpu.zoom_enabled ? "on" : "off") << std::endl;
                                break;
                            case 2:
                                cpu.info_osd = !cpu.info_osd;
                                std::cout << "[UI] osd info: " << (cpu.info_osd ? "on" : "off") << std::endl;
                                break;
                            case 3:
                                cpu.audio_track = (cpu.audio_track == 1) ? 2 : 1;
                                std::cout << "[UI] audio track: " << cpu.audio_track << std::endl;
                                break;
                            case 4:
                                cpu.subtitle_on = !cpu.subtitle_on;
                                std::cout << "[UI] subtitles: " << (cpu.subtitle_on ? "on" : "off") << std::endl;
                                break;
                            case 5:
                                if (dir == 1 && cpu.current_chapter < cpu.total_chapters) {
                                    cpu.current_chapter++;
                                    cpu.regs[0] = cpu.chapter_start_frame[cpu.current_chapter-1] * (SCREEN_WIDTH*SCREEN_HEIGHT*3/2);
                                    cpu.regs[4] = cpu.chapter_start_frame[cpu.current_chapter-1];
                                }
                                if (dir == -1 && cpu.current_chapter > 1) {
                                    cpu.current_chapter--;
                                    cpu.regs[0] = cpu.chapter_start_frame[cpu.current_chapter-1] * (SCREEN_WIDTH*SCREEN_HEIGHT*3/2);
                                    cpu.regs[4] = cpu.chapter_start_frame[cpu.current_chapter-1];
                                }
                                std::cout << "[UI] chapter: " << cpu.current_chapter << "/" << cpu.total_chapters << std::endl;
                                break;
                        }
                    }
                }
            }
        }

        // автопереход между главами
        if (cpu.state == AdvancedDVDProcessor::PLAYING && cpu.has_disc) {
            int chap_end = (cpu.current_chapter < cpu.total_chapters) ?
                cpu.chapter_start_frame[cpu.current_chapter] : 1500;
            if (cpu.regs[4] >= chap_end) {
                if (cpu.current_chapter < cpu.total_chapters) {
                    cpu.current_chapter++;
                    cpu.regs[0] = cpu.chapter_start_frame[cpu.current_chapter-1] * (SCREEN_WIDTH*SCREEN_HEIGHT*3/2);
                    cpu.regs[4] = cpu.chapter_start_frame[cpu.current_chapter-1];
                    std::cout << "[PLAYER] auto next chapter: " << cpu.current_chapter << "/" << cpu.total_chapters << std::endl;
                } else {
                    cpu.state = AdvancedDVDProcessor::STOPPED;
                    cpu.regs[4] = 0;
                    cpu.regs[0] = 0;
                    cpu.current_chapter = 1;
                    cpu.clear_screen();
                    std::cout << "[PLAYER] movie finished, stopped" << std::endl;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(cpu.vram_mutex);
            for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
                pixels[i*4]   = cpu.VRAM[i*3];
                pixels[i*4+1] = cpu.VRAM[i*3+1];
                pixels[i*4+2] = cpu.VRAM[i*3+2];
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
