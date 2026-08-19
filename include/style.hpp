#pragma once 
#include <ostream>

// src/detail_ansiCodes.cpp
struct g_StyleConfig{
    inline static bool disabled {false};
    inline static bool isDisabled(){
        return disabled;
    }
    inline static bool isEnabled(){
        return !disabled;
    }
    inline static void disable(){
        disabled = true;
    }
    inline static void enable(){
        disabled = false;
    }
};


namespace style{
namespace detail_ansi{
    enum {
    reset=0,
    bold=1,
    dim=2,
    italic=3,
    underline=4,
    slow_blink=5,
    fast_blink=6,
    rev=7,
    hide=8,
    strike=9,
    dbl_underline=21,
    no_intensity=22,
    no_italic=23,
    no_under=24,
    no_blink=25,
    no_rev=27,
    no_hide=28,
    no_strike=29,

    fg_black=30,
    fg_red=31,
    fg_green=32,
    fg_yellow=33,
    fg_blue=34,
    fg_magenta=35,
    fg_cyan=36,
    fg_white=37,

    //fg_rgb(...)=38,

    fg_default=39,
    bg_black=40,
    bg_red=41,
    bg_green=42,
    bg_yellow=43,
    bg_blue=44,
    bg_magenta=45,
    bg_cyan=46,
    bg_white=47,

    //bg_rgb(...)=48,

    bg_default=49,

    framed=51,
    circle=52,
    overline=53,
    no_frame=54,
    no_over=55,

    //un_rgb(...)=58,

    un_default=59,
    sup=73,
    sub=74,
    no_su=75,

    bg_grey=100,
    bg_br_red=101,
    bg_br_green=102,
    bg_br_yellow=103,
    bg_br_blue=104,
    bg_br_magenta=105,
    bg_br_cyan=106,

    fg_grey=90,
    fg_br_red=91,
    fg_br_green=92,
    fg_br_yellow=93,
    fg_br_blue=94,
    fg_br_magenta=95,
    fg_br_cyan=96,
};

inline std::string fg_rgb(int r, int g, int b){
    constexpr static std::string_view fg_rgb_prefix = "38;2";
    return std::format(
        "{};{};{};{}", 
        fg_rgb_prefix,
        std::to_string(r),
        std::to_string(g),
        std::to_string(b)
    );
}


inline std::string bg_rgb(int r, int g, int b){
    constexpr static std::string_view bg_rgb_prefix = "48;2";
    return std::format(
        "{};{};{};{}", 
        bg_rgb_prefix,
        std::to_string(r),
        std::to_string(g),
        std::to_string(b)
    );
}

inline std::string underline_rgb(int r, int g, int b){
    constexpr static std::string_view underline_rgb_prefix = "58;2";
    return std::format(
        "{};{};{};{}", 
        underline_rgb_prefix,
        std::to_string(r),
        std::to_string(g),
        std::to_string(b)
    );
}

inline std::string fg_rgb(int w){ return fg_rgb(w,w,w); }
inline std::string bg_rgb(int w){ return bg_rgb(w,w,w); }
inline std::string underline_rgb(int w){ return underline_rgb(w,w,w); }


inline std::string bg_code_blocks = detail_ansi::bg_rgb(50);
} //NOTE: namespace: detail_ansi

template<typename ...Args>
inline std::string make_style(Args ...codes){
    if (g_StyleConfig::disabled){
        return "";
    }
    if constexpr(sizeof...(codes) ==0) return "\e[m";
    using namespace std;
    string out{"\e["};
    auto append =[&](auto code){
        if constexpr (std::is_same_v<std::string, decltype(code)>){
            out += code + ";";
        }else{
            out += to_string(code) + ";";
        }
    };
    (append(codes), ...);
    out.back()='m';
    return out;
}

template<typename ...Args>
inline std::string bg_rgb(Args ...vargs){
    if constexpr(sizeof...(vargs) ==0) return "\e[m";
    using namespace std;
    string out{"\e["};
    auto append =[&](auto code){
        out+=to_string(code) + ";";
    };
    (append(vargs), ...);
    out.back()='m';
    return out;
}

inline std::string clear_row(){return "\e[2K";}
inline std::string up_row(){return "\e[A";}
inline std::string up_rows(int count){
    if (count <= 0) return "";
    return up_row() + up_rows(count-1);
}
inline std::string fg_rgb(int r, int g, int b){return make_style(detail_ansi::fg_rgb(r,g,b));}
inline std::string fg_rgb(int w){return make_style(detail_ansi::fg_rgb(w));}
inline std::string bg_rgb(int r, int g, int b){return make_style(detail_ansi::bg_rgb(r,g,b));}
inline std::string bg_rgb(int w){return make_style(detail_ansi::bg_rgb(w));}
                                                               
        inline std::string reset(){return make_style(detail_ansi::reset);          }
         inline std::string bold(){return make_style(detail_ansi::bold);           }
          inline std::string dim(){return make_style(detail_ansi::dim);            }
       inline std::string italic(){return make_style(detail_ansi::italic);         }
    inline std::string underline(){return make_style(detail_ansi::underline);      }
   inline std::string slow_blink(){return make_style(detail_ansi::slow_blink);     }
   inline std::string fast_blink(){return make_style(detail_ansi::fast_blink);     }
          inline std::string rev(){return make_style(detail_ansi::rev);            }
         inline std::string hide(){return make_style(detail_ansi::hide);           }
       inline std::string strike(){return make_style(detail_ansi::strike);         }
inline std::string dbl_underline(){return make_style(detail_ansi::dbl_underline);  }
 inline std::string no_intensity(){return make_style(detail_ansi::no_intensity);   }
    inline std::string no_italic(){return make_style(detail_ansi::no_italic);      }
     inline std::string no_under(){return make_style(detail_ansi::no_under);       }
     inline std::string no_blink(){return make_style(detail_ansi::no_blink);       }
       inline std::string no_rev(){return make_style(detail_ansi::no_rev);         }
      inline std::string no_hide(){return make_style(detail_ansi::no_hide);        }
    inline std::string no_strike(){return make_style(detail_ansi::no_strike);      }
     inline std::string fg_black(){return make_style(detail_ansi::fg_black);       }
       inline std::string fg_red(){return make_style(detail_ansi::fg_red);         }
     inline std::string bold_red(){return make_style(detail_ansi::bold, detail_ansi::fg_rgb(255,0,0));}
     inline std::string fg_green(){return make_style(detail_ansi::fg_green);       }
    inline std::string fg_yellow(){return make_style(detail_ansi::fg_yellow);      }
      inline std::string fg_blue(){return make_style(detail_ansi::fg_blue);        }
   inline std::string fg_magenta(){return make_style(detail_ansi::fg_magenta);     }
      inline std::string fg_cyan(){return make_style(detail_ansi::fg_cyan);        }
     inline std::string fg_white(){return make_style(detail_ansi::fg_white);       }
      inline std::string fg_pink(){return make_style(detail_ansi::fg_rgb(255,192,203));}
   inline std::string fg_default(){return make_style(detail_ansi::fg_default);     }
     inline std::string reset_fg(){return make_style(detail_ansi::fg_default);     }
     inline std::string bg_black(){return make_style(detail_ansi::bg_black);       }
       inline std::string bg_red(){return make_style(detail_ansi::bg_red);         }
     inline std::string bg_green(){return make_style(detail_ansi::bg_green);       }
    inline std::string bg_yellow(){return make_style(detail_ansi::bg_yellow);      }
      inline std::string bg_blue(){return make_style(detail_ansi::bg_blue);        }
   inline std::string bg_magenta(){return make_style(detail_ansi::bg_magenta);     }
      inline std::string bg_cyan(){return make_style(detail_ansi::bg_cyan);        }
     inline std::string bg_white(){return make_style(detail_ansi::bg_white);       }
   inline std::string bg_default(){return make_style(detail_ansi::bg_default);     }
     inline std::string reset_bg(){return make_style(detail_ansi::bg_default);     }
       inline std::string framed(){return make_style(detail_ansi::framed);         }
       inline std::string circle(){return make_style(detail_ansi::circle);         }
     inline std::string overline(){return make_style(detail_ansi::overline);       }
     inline std::string no_frame(){return make_style(detail_ansi::no_frame);       }
      inline std::string no_over(){return make_style(detail_ansi::no_over);        }
   inline std::string un_default(){return make_style(detail_ansi::un_default);     }
          inline std::string sup(){return make_style(detail_ansi::sup);            }
          inline std::string sub(){return make_style(detail_ansi::sub);            }
        inline std::string no_su(){return make_style(detail_ansi::no_su);          }
      inline std::string bg_grey(){return make_style(detail_ansi::bg_grey);        }
      inline std::string bg_gray(){return make_style(detail_ansi::bg_grey);        }
      inline std::string fg_grey(){return make_style(detail_ansi::fg_grey);        }
      inline std::string fg_gray(){return make_style(detail_ansi::fg_grey);        }
    inline std::string bg_br_red(){return make_style(detail_ansi::bg_br_red);      }
  inline std::string bg_br_green(){return make_style(detail_ansi::bg_br_green);    }
 inline std::string bg_br_yellow(){return make_style(detail_ansi::bg_br_yellow);   }
   inline std::string bg_br_blue(){return make_style(detail_ansi::bg_br_blue);     }
inline std::string bg_br_magenta(){return make_style(detail_ansi::bg_br_magenta);  }
   inline std::string bg_br_cyan(){return make_style(detail_ansi::bg_br_cyan);     }
    inline std::string fg_br_red(){return make_style(detail_ansi::fg_br_red);      }
  inline std::string fg_br_green(){return make_style(detail_ansi::fg_br_green);    }
 inline std::string fg_br_yellow(){return make_style(detail_ansi::fg_br_yellow);   }
   inline std::string fg_br_blue(){return make_style(detail_ansi::fg_br_blue);     }
inline std::string fg_br_magenta(){return make_style(detail_ansi::fg_br_magenta);  }
   inline std::string fg_br_cyan(){return make_style(detail_ansi::fg_br_cyan);     }
    ////////////////////////////////////////////////////////////////////////////

inline auto bg_code()     { return style::make_style(detail_ansi::bg_code_blocks);           }
inline auto fg_type()       { return style::make_style(detail_ansi::fg_rgb(243, 211, 152));      }
inline auto fg_val()        { return style::make_style(detail_ansi::fg_rgb(255, 165, 119));       }
inline auto fg_identifier() { return style::make_style(detail_ansi::fg_rgb(252, 148, 159));}

template<typename T>
std::string with(auto style, const T& val){
    return std::format("{0}{1}{2}",style,val,style::reset());
}
template<typename T>
std::string with_fg(auto style, const T& val){
    return std::format("{0}{1}{2}",style,val,style::reset_fg());
}
template<typename T>
std::string with_bg(auto style, const T& val){
    return std::format("{0}{1}{2}",style,val,style::reset_bg());
}
}
