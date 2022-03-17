//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

enum net_sample_type { bandwidth_in = 0, bandwidth_out = 1 };

struct net_sample
{
    double _start_time;        // timestamp of beginning of first sample (interval begin)
    double _end_time;          // timestamp of end of last sample (interval end)
    double _max_in_bandwidth;  // highest recorded bandwidth noticed during interval
    double _max_out_bandwidth; // highest recorded bandwidth noticed during interval
    uint64 _total_in_bytes;    // total bytes received during interval
    uint64 _total_out_bytes;   // total bytes sent during interval

    void reset(double current_time);
    void add_sample(uint64 in_bytes, uint64 out_bytes, double current_time);
    void finalize(double current_time);
    double time_span() const;
    double average(uint64 bytes) const;
    double get_average(net_sample_type type) const;
};

//////////////////////////////////////////////////////////////////////

struct network_stats
{
    int _num_bars;                 // total bars in the graph, regardless of how many drawn
    std::vector<net_sample> _bars; // the bars in the bar graph
    std::mutex _mutex;             // for switching to a new bar, don't draw while that's happening
    MMRESULT _timer_id;            // sampler timer
    double _bar_timespan;          // how much time for each bar
    double _base_time;             // local epoch
    double _file_base_time;        // when we started recording
    double _last_sample_time;      // last sample started at this time
    uint64 _current_bar;           // which one being sampled into now
    uint64 _total_samples;         // # of samples gathered so far
    uint64 _global_in;
    uint64 _global_out;
    double _sample_time;

    double _file_last_write_time;
    net_sample _file_sample;

    MIB_IF_ROW2 *_adapter;

    HANDLE _output_file;

    network_stats(double bar_timespan, int num_bars);
    void start(int sample_delay_ms, MIB_IF_ROW2 *adapter, double samples_per_second);
    void update(double seconds_per_sample);
    void stop();

    bool start_recording(wchar const *filename);
    void stop_recording();

    int get_current_bar_index() const
    {
        return _current_bar % _num_bars;
    }
};

wchar const *get_interface_type_name(int t);
