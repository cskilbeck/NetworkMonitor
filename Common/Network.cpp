//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

void net_sample::reset(double start_time)
{
    _start_time = start_time;
    _max_in_bandwidth = 0;
    _max_out_bandwidth = 0;
    _total_in_bytes = 0;
    _total_out_bytes = 0;
}

//////////////////////////////////////////////////////////////////////

void net_sample::finalize(double cur_time)
{
    double total_sample_time = cur_time - _start_time;
    double in_bandwidth = calc_bandwidth(_total_in_bytes, total_sample_time);
    double out_bandwidth = calc_bandwidth(_total_out_bytes, total_sample_time);
    _max_in_bandwidth = (std::max)(_max_in_bandwidth, in_bandwidth);
    _max_out_bandwidth = (std::max)(_max_out_bandwidth, out_bandwidth);
    _end_time = cur_time;
}

//////////////////////////////////////////////////////////////////////

void net_sample::add_sample(uint64 in_bytes, uint64 out_bytes, double current_time)
{
    _total_in_bytes += in_bytes;
    _total_out_bytes += out_bytes;
}

//////////////////////////////////////////////////////////////////////

double net_sample::time_span() const
{
    return _end_time - _start_time;
}

//////////////////////////////////////////////////////////////////////

double net_sample::average(uint64 bytes) const
{
    return calc_bandwidth(bytes, time_span());
}

//////////////////////////////////////////////////////////////////////

double net_sample::get_average(net_sample_type type) const
{
    switch(type) {
    case bandwidth_in:
        return average(_total_in_bytes);
    case bandwidth_out:
        return average(_total_out_bytes);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////

network_stats::network_stats(double desired_bar_timespan, int num_bars)
    : _num_bars(num_bars)
    , _bars(_num_bars)
    , _bar_timespan(desired_bar_timespan)
    , _current_bar(0)
    , _last_sample_time(0)
    , _timer_id(0)
    , _global_out(0)
    , _global_in(0)
    , _total_samples(0)
    , _adapter(null)
    , _output_file(INVALID_HANDLE_VALUE)
{
}

//////////////////////////////////////////////////////////////////////

static void CALLBACK timer_function(uint timer_id, uint msg, DWORD_PTR user_value, DWORD_PTR dword_1, DWORD_PTR dword_2)
{
    network_stats *g = reinterpret_cast<network_stats *>(user_value);
    g->update(g->_sample_time);
}

//////////////////////////////////////////////////////////////////////

void network_stats::start(int sample_delay_ms, MIB_IF_ROW2 *adapter, double samples_per_second)
{
    stop();
    _current_bar = 0;
    _last_sample_time = 0;
    _timer_id = 0;
    _global_out = 0;
    _global_in = 0;
    _total_samples = 0;
    _adapter = adapter;
    _sample_time = samples_per_second;
    _base_time = time_now();
    uint options = TIME_PERIODIC | TIME_KILL_SYNCHRONOUS;
    _timer_id = timeSetEvent(sample_delay_ms, 0, timer_function, reinterpret_cast<DWORD_PTR>(this), options);
}

//////////////////////////////////////////////////////////////////////

void network_stats::stop()
{
    stop_recording();
    if(_timer_id != 0) {
        timeKillEvent(_timer_id);
        _timer_id = 0;
    }
}

//////////////////////////////////////////////////////////////////////

void network_stats::update(double seconds_per_sample)
{
    if(FAILED(GetIfEntry2(_adapter))) {
        return;
    }

    double current_time = time_now() - _base_time;
    double seconds_per_bar = 1.0 / seconds_per_sample;
    auto cur_bar = _bars.begin() + _current_bar % _num_bars;

    // time since graph epoch
    double now = current_time;

    // first time through just primes the totals
    if(_total_samples != 0) {

        // time in this particular bar
        double time_delta = now - cur_bar->_start_time;

        // enough time spent in this bar?
        if(time_delta > seconds_per_bar) {
            // yes, lock because we're updating _current_bar which draw function needs to know about
            std::lock_guard<std::mutex> lock(_mutex);

            // finished with this one
            cur_bar->finalize(now);

            // start a new one
            _current_bar += 1;
            cur_bar = _bars.begin() + _current_bar % _num_bars;
            cur_bar->reset(now);
        }

        // in any case, add this sample to the current bar
        cur_bar->add_sample(_adapter->InOctets - _global_in, _adapter->OutOctets - _global_out, now);

        if(_output_file != INVALID_HANDLE_VALUE) {

            double file_delta = now - _file_sample._start_time;

            if(file_delta >= 0.1) { // seconds for a row in the csv

                _file_sample.finalize(now);
                std::string line = format("%f,%lld,%lld\r\n",
                    now - _file_base_time,
                    _file_sample._total_in_bytes,
                    _file_sample._total_out_bytes);
                WriteFile(_output_file, line.c_str(), line.size(), null, null);
                _file_sample.reset(now);
            }
            _file_sample.add_sample(_adapter->InOctets - _global_in, _adapter->OutOctets - _global_out, now);
        }
    }

    _last_sample_time = now;
    _total_samples += 1;
    _global_in = _adapter->InOctets;
    _global_out = _adapter->OutOctets;
}

//////////////////////////////////////////////////////////////////////

bool network_stats::start_recording(wchar const *filename)
{
    stop_recording();

    auto attr = FILE_ATTRIBUTE_NORMAL;
    auto share = FILE_SHARE_READ;
    auto perm = GENERIC_WRITE;
    auto disposition = CREATE_ALWAYS;
    _output_file = CreateFileW(filename, perm, share, null, disposition, attr, null);
    if(_output_file == INVALID_HANDLE_VALUE) {
        auto err = WindowsError(L"Can't open %s for writing", filename);
        MessageBox(null, err.c_str(), L"NetworkMonitor", MB_ICONEXCLAMATION);
        return false;
    }
    else {
        std::string line = format("#T,IN,OUT\r\n0.0,0,0\r\n");
        WriteFile(_output_file, line.c_str(), line.size(), null, null);
    }
    double now = time_now() - _base_time;
    _file_sample.reset(now);
    _file_base_time = now;
    return true;
}

//////////////////////////////////////////////////////////////////////

void network_stats::stop_recording()
{
    if(_output_file != null) {
        CloseHandle(_output_file);
        _output_file = null;
    }
}
