#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <iostream>
#include <chrono>

struct ChildProc {
    pid_t pid = -1;
    int in_fd = -1;
    int out_fd = -1;
};

static void set_nonblock(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static ChildProc spawn_core(const char* path){
    int inpipe[2];
    int outpipe[2];
    pipe(inpipe);
    pipe(outpipe);

    pid_t pid = fork();
    if(pid == 0){
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);
        dup2(outpipe[1], STDERR_FILENO);
        close(inpipe[1]); close(outpipe[0]);
        execl(path, path, (char*)nullptr);
        _exit(127);
    }

    close(inpipe[0]);
    close(outpipe[1]);
    set_nonblock(outpipe[0]);

    ChildProc cp;
    cp.pid = pid;
    cp.in_fd = inpipe[1];
    cp.out_fd = outpipe[0];
    return cp;
}

static void child_send(const ChildProc& cp, const std::string& line){
    std::string s = line;
    if(s.empty() || s.back() != '\n') s.push_back('\n');
    ::write(cp.in_fd, s.c_str(), s.size());
}

static void child_read_lines(const ChildProc& cp, std::deque<std::string>& outLines, size_t maxKeep=500){
    char buf[4096];
    while(true){
        ssize_t n = ::read(cp.out_fd, buf, sizeof(buf)-1);
        if(n <= 0) break;
        buf[n] = 0;
        static std::string carry;
        carry += buf;
        size_t pos;
        while((pos = carry.find('\n')) != std::string::npos){
            std::string line = carry.substr(0,pos);
            carry.erase(0,pos+1);
            outLines.push_back(line);
            if(outLines.size() > maxKeep) outLines.pop_front();
        }
    }
}

// --- Minimal 8x8 font (fallback; box glyphs) ---
static unsigned char glyph8x8(int /*c*/, int row){
    // simple rectangle border in 8x8
    if(row==0 || row==7) return 0xFF;
    return 0x81;
}

struct Canvas {
    int w=0,h=0;
    std::vector<unsigned int> px; // ARGB
    void resize(int W,int H){
        w=W; h=H;
        px.assign((size_t)w*(size_t)h, 0xFF101018);
    }
    inline void clear(unsigned int c){ std::fill(px.begin(), px.end(), c); }
    inline void put(int x,int y,unsigned int c){
        if((unsigned)x<(unsigned)w && (unsigned)y<(unsigned)h) px[(size_t)y*(size_t)w + (size_t)x] = c;
    }
};

static void rect(Canvas& c, int x,int y,int w,int h, unsigned int col){
    for(int yy=y; yy<y+h; ++yy){
        for(int xx=x; xx<x+w; ++xx) c.put(xx,yy,col);
    }
}

static void draw_char(Canvas& c, int x,int y, char ch, unsigned int col){
    unsigned char uc = (unsigned char)ch;
    for(int row=0; row<8; ++row){
        unsigned char bits = glyph8x8(uc,row);
        for(int colbit=0; colbit<8; ++colbit){
            if(bits & (1 << (7-colbit))) c.put(x+colbit, y+row, col);
        }
    }
}

static void text(Canvas& c, int x,int y, const std::string& s, unsigned int col){
    int cx=x;
    for(char ch: s){
        if(ch=='\t'){ cx += 8*4; continue; }
        draw_char(c,cx,y,ch,col);
        cx += 8;
    }
}

struct Button { int x,y,w,h; std::string label; };

static bool hit(const Button& b, int mx,int my){
    return mx>=b.x && mx<=b.x+b.w && my>=b.y && my<=b.y+b.h;
}

int main(){
    Display* dpy = XOpenDisplay(nullptr);
    if(!dpy){ std::cerr<<"XOpenDisplay failed\\n"; return 1; }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    int width=1200, height=720;

    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
    Window win = XCreateWindow(dpy, root, 0,0,width,height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &swa);
    XStoreName(dpy, win, "Rael Cockpit (Phase 6A) — Shell + Test + Telemetry");
    XMapWindow(dpy, win);

    Visual* visual = DefaultVisual(dpy, screen);
    int depth = DefaultDepth(dpy, screen);
    GC gc = XCreateGC(dpy, win, 0, nullptr);

    Canvas canvas; canvas.resize(width,height);
    XImage* img = XCreateImage(dpy, visual, depth, ZPixmap, 0, (char*)canvas.px.data(), width, height, 32, 0);

    // spawn core
    ChildProc core = spawn_core("./rael");
    std::deque<std::string> coreOut;
    child_send(core, "help");
    child_send(core, "status");

    int tab=0; // 0 overview,1 shell,2 test
    std::string input;
    bool running=true;
    int mx=0,my=0;

    Button tabOverview{10,10,140,36,"OVERVIEW"};
    Button tabShell{160,10,140,36,"SHELL"};
    Button tabTest{310,10,140,36,"TEST"};

    Button bSchedBal{30,90,220,44,"scheduler balanced"};
    Button bSchedSem{30,140,220,44,"scheduler semantic"};
    Button bSchedThr{30,190,220,44,"scheduler throughput"};

    Button bAmpOff{280,90,220,44,"amplifier off"};
    Button bAmpAd{280,140,220,44,"amplifier adaptive"};
    Button bAmpFo{280,190,220,44,"amplifier forced"};

    auto lastPoll = std::chrono::steady_clock::now();

    while(running){
        while(XPending(dpy)){
            XEvent xev; XNextEvent(dpy,&xev);
            if(xev.type == ConfigureNotify){
                width = xev.xconfigure.width;
                height = xev.xconfigure.height;
                canvas.resize(width,height);
                // recreate image with new buffer
                img = XCreateImage(dpy, visual, depth, ZPixmap, 0, (char*)canvas.px.data(), width, height, 32, 0);
            } else if(xev.type == MotionNotify){
                mx = xev.xmotion.x; my = xev.xmotion.y;
            } else if(xev.type == ButtonPress){
                mx = xev.xbutton.x; my = xev.xbutton.y;
                if(hit(tabOverview,mx,my)) tab=0;
                else if(hit(tabShell,mx,my)) tab=1;
                else if(hit(tabTest,mx,my)) tab=2;

                if(tab==2){
                    if(hit(bSchedBal,mx,my)) child_send(core, "set scheduler balanced");
                    if(hit(bSchedSem,mx,my)) child_send(core, "set scheduler semantic");
                    if(hit(bSchedThr,mx,my)) child_send(core, "set scheduler throughput");

                    if(hit(bAmpOff,mx,my)) child_send(core, "set amplifier off");
                    if(hit(bAmpAd,mx,my)) child_send(core, "set amplifier adaptive");
                    if(hit(bAmpFo,mx,my)) child_send(core, "set amplifier forced");

                    // Avoid depending on a non-universal "get" command.
                    // Pull a quick snapshot using the standard read commands.
                    child_send(core, "node");
                    child_send(core, "lane");
                    child_send(core, "events 8");
                }
            } else if(xev.type == KeyPress){
                KeySym ks;
                char buf[64];
                int len = XLookupString(&xev.xkey, buf, sizeof(buf), &ks, nullptr);
                if(ks == XK_Escape){ running=false; }
                else if(ks == XK_Return){
                    if(!input.empty()){
                        child_send(core, input);
                        input.clear();
                    }
                } else if(ks == XK_BackSpace){
                    if(!input.empty()) input.pop_back();
                } else if(ks == XK_Tab){
                    tab = (tab+1)%3;
                } else {
                    for(int i=0;i<len;i++){
                        if(buf[i]>=32 && buf[i]<=126) input.push_back(buf[i]);
                    }
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now-lastPoll).count() > 350){
            lastPoll = now;
            child_send(core, "lane");
            child_send(core, "node");
            child_send(core, "events 8");
        }

        child_read_lines(core, coreOut);

        // Render
        canvas.clear(0xFF0F1016);
        // Top bar
        rect(canvas,0,0,width,56,0xFF1A1B23);

        auto draw_tab=[&](const Button& b, bool active){
            rect(canvas,b.x,b.y,b.w,b.h, active?0xFF3A3B4A:0xFF2A2B35);
            text(canvas,b.x+8,b.y+12,b.label,0xFFFFFFFF);
        };
        draw_tab(tabOverview, tab==0);
        draw_tab(tabShell, tab==1);
        draw_tab(tabTest, tab==2);

        rect(canvas,0,56,width,height-56,0xFF141521);

        if(tab==0){
            text(canvas,20,80,"RAEL — LIVE OVERVIEW (Phase 6A)",0xFFFFFFFF);
            text(canvas,20,100,"Polling: lane + node + events. Use TEST for toggles. SHELL for full control.",0xFFDDDDFF);

            int y=130, shown=0;
            for(auto it=coreOut.rbegin(); it!=coreOut.rend() && shown<26; ++it,++shown){
                text(canvas,20,y,*it,0xFFCFD1FF);
                y += 10;
            }
        } else if(tab==1){
            text(canvas,20,80,"/rael SHELL (embedded)",0xFFFFFFFF);
            rect(canvas,20,100,width-40,height-170,0xFF0E0F16);

            int linesToShow = (height-190)/10;
            int start = (int)coreOut.size() - linesToShow;
            if(start < 0) start = 0;
            int y=110;
            for(int i=start; i<(int)coreOut.size(); ++i){
                text(canvas,30,y,coreOut[i],0xFFCFD1FF);
                y += 10;
            }

            rect(canvas,20,height-60,width-40,30,0xFF1A1B23);
            text(canvas,30,height-52,"> "+input,0xFFFFFFFF);

        } else if(tab==2){
            text(canvas,20,80,"TEST PANEL (toggles via /rael set ...)",0xFFFFFFFF);

            auto draw_btn=[&](const Button& b){
                rect(canvas,b.x,b.y,b.w,b.h,0xFF2A2B35);
                text(canvas,b.x+8,b.y+16,b.label,0xFFFFFFFF);
            };
            draw_btn(bSchedBal); draw_btn(bSchedSem); draw_btn(bSchedThr);
            draw_btn(bAmpOff); draw_btn(bAmpAd); draw_btn(bAmpFo);

            int y=260, shown=0;
            for(auto it=coreOut.rbegin(); it!=coreOut.rend() && shown<18; ++it,++shown){
                text(canvas,30,y,*it,0xFFCFD1FF);
                y += 10;
            }
        }

        XPutImage(dpy, win, gc, img, 0,0, 0,0, width, height);
        XFlush(dpy);
        usleep(16*1000);
    }

    child_send(core, "exit");
    int status=0;
    waitpid(core.pid, &status, 0);

    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}
