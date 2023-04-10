#include "VRV32ICore.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace std;

struct Config {
    size_t _max_trace_depth{99};
    size_t _max_sim_time{10000};
    string _bin_file{"a.bin"}, _vcd_file{"a.vcd"};
    size_t _text_base{0x0}, _data_base{0x10000};
    char **_begin{nullptr}, **_end{nullptr};

    Config(char **begin, char **end) : _begin(begin), _end(end) {
        string opt;
        opt = get_cmd_option("-textbase").value_or("0x0");
        _text_base = stoull(opt);
        opt = get_cmd_option("-database").value_or("0x10000");
        _data_base = stoull(opt);
        _bin_file = get_cmd_option("-bin").value_or("a.bin");
        auto dot_pos = _bin_file.find_last_of('.');
        auto default_vcd_file = _bin_file.substr(0, dot_pos) + ".vcd";
        _vcd_file = get_cmd_option("-vcd").value_or(default_vcd_file);
    }
    optional<string> get_cmd_option(const string &opt) {
        auto it = find(_begin, _end, opt);
        if (it == _end) {
            return {};
        }
        return {it[1]};
    }
};

int main(int argc, char *argv[]) {
    auto cfg = Config(argv, argv + argc);

    Verilated::traceEverOn(true);

    shared_ptr<VRV32ICore> rv{new VRV32ICore};
    shared_ptr<VerilatedContext> ctx{new VerilatedContext};
    ctx->commandArgs(argc, argv);

    shared_ptr<VerilatedVcdC> vcd{new VerilatedVcdC};
    rv->trace(vcd.get(), cfg._max_trace_depth);
    cout << "Dumping waveform to " + cfg._vcd_file + '\n';
    vcd->open(cfg._vcd_file.c_str());

    auto step_one_cycle = [&](bool dump = true, bool time_inc = true) {
        for (auto i = 0; i < 2; i++) {
            rv->eval();
            if (dump) {
                vcd->dump(ctx->time());
            }
            if (time_inc) {
                ctx->timeInc(1);
            }
            rv->CPU_CLK ^= 1;
        }
    };

    // init input to top module
    // set reset when loding data to cache
    rv->CPU_RST = 1;
    rv->CPU_CLK = 0;
    rv->CPU_Debug_InstCache_WE2 = 0;
    rv->CPU_Debug_DataCache_WE2 = 0;

    // load bin file to i$ / d$
    auto write_cache = [&](size_t addr, size_t word) {
        // i$
        rv->CPU_Debug_InstCache_WE2 = 0xf;
        rv->CPU_Debug_InstCache_A2 = addr;
        rv->CPU_Debug_InstCache_WD2 = word;
        // d$
        rv->CPU_Debug_DataCache_WE2 = 0xf;
        rv->CPU_Debug_DataCache_A2 = addr;
        rv->CPU_Debug_DataCache_WD2 = word;

        step_one_cycle(false, false);

        rv->CPU_Debug_InstCache_WE2 = 0;
        rv->CPU_Debug_DataCache_WE2 = 0;
    };
    ifstream bin{cfg._bin_file, ifstream::binary};
    vector<unsigned char> bin_buf(istreambuf_iterator<char>{bin},
                                  istreambuf_iterator<char>());
    for (size_t i = 0; i < bin_buf.size(); i += 4) {
        auto word = 0x0u;
        for (size_t j = 0; j < 4 && i + j < bin_buf.size(); j++) {
            word |= static_cast<size_t>(bin_buf[i + j]) << (j * 8);
        }
        write_cache(cfg._text_base + i, word);
    }

    // reset
    rv->CPU_RST = 1;
    step_one_cycle();
    rv->CPU_RST = 0;

    // run
    while (not ctx->gotFinish() && ctx->time() < cfg._max_sim_time) {
        step_one_cycle();
    }

    vcd->close();

    return 0;
}