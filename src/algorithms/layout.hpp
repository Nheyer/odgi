#include <iostream>
#include <sdsl/enc_vector.hpp>
#include <handlegraph/handle_graph.hpp>
#include <handlegraph/util.hpp>
#include "atomic_image.hpp"

namespace odgi {

namespace algorithms {

namespace layout {

using namespace handlegraph;

void to_tsv(std::ostream &out,
            const std::vector<double> &X,
            const std::vector<double> &Y,
            const HandleGraph &graph);

union conv_t { uint64_t i; double d; };

class Layout {
    sdsl::enc_vector<> xy;
    double min_value = std::numeric_limits<double>::max();
public:
    Layout(void) { }
    Layout(const std::vector<double> &X, const std::vector<double> &Y);
    void serialize(std::ostream& out);
    void load(std::istream& in);
    void to_tsv(std::ostream &out);
    xy_d_t coords(const handle_t& handle);
    size_t size(void);
    double get_x(uint64_t i);
    double get_y(uint64_t i);
    std::vector<double> get_X(void);
    std::vector<double> get_Y(void);
};

}

}

}
