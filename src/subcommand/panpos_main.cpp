#include "subcommand.hpp"
#include "args.hxx"
#include "algorithms/xp.hpp"

namespace odgi {

    using namespace odgi::subcommand;
    using namespace xp;

    int main_panpos(int argc, char** argv) {

        for (uint64_t i = 1; i < argc-1; ++i) {
            argv[i] = argv[i+1];
        }
        const std::string prog_name = "odgi panpos";
        argv[0] = (char*)prog_name.c_str();
        --argc;

        args::ArgumentParser parser("get the pangenome position of a given path and nucleotide position (1-based)");
        args::HelpFlag help(parser, "help", "display this help summary", {'h', "help"});
        args::ValueFlag<std::string> dg_in_file(parser, "FILE", "load the index from this file", {'i', "idx"});
        args::ValueFlag<std::string> path_name(parser, "STRING", "get the pangenome position of this path", {'p', "path"});
        args::ValueFlag<uint64_t> nuc_pos(parser, "N", "get the pangenome position of this nucleotide position", {'n', "nuc-pos"});

        try {
            parser.ParseCLI(argc, argv);
        } catch (args::Help) {
            std::cout << parser;
            return 0;
        } catch (args::ParseError e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return 1;
        }
        if (argc==1) {
            std::cout << parser;
            return 1;
        }

        if (!dg_in_file) {
            std::cerr << "[odgi panpos] error: Please enter a file to read the index from via -i=[FILE], --idx=[FILE]." << std::endl;
            exit(1);
        }
        if (!path_name) {
            std::cerr << "[odgi panpos] error: Please enter a valid path name to extract the pangenome position from via -p=[STRING], --path=[STRING]." << std::endl;
            exit(1);
        }
        if (!nuc_pos) {
            std::cerr << "[odgi panpos] error: Please enter a valid nucleotide position to extract the corresponding pangenome position from -n=[N], --nuc-pos=[N]." << std::endl;
            exit(1);
        }

        XP path_index;
        std::ifstream in;
        in.open(args::get(dg_in_file));
        path_index.load(in);
        in.close();

        // we have a 0-based positioning
        uint64_t nucleotide_pos = args::get(nuc_pos) - 1;
        std::string p_name = args::get(path_name);

        if (!path_index.has_path(p_name)) {
            std::cerr << "The given path name " << p_name << " is not in the index." << std::endl;
            exit(1);
        }

        if (!path_index.has_position(p_name, nucleotide_pos)) {
            std::cerr << "The given path " << p_name << " with nucleotide position " << args::get(nuc_pos) << " is not in the index." << std::endl;
            exit(1);
        }

        size_t pangenome_pos = path_index.get_pangenome_pos(p_name, nucleotide_pos) + 1;
        std::cout << pangenome_pos << std::endl;

        return 0;
    }

    static Subcommand odgi_panpos("panpos",
            "get the pangenome position for a given path and nucleotide position (1-based)",
                                     PIPELINE, 3, main_panpos);

}
