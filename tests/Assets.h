#pragma once

#include <noa/common/Types.h>
#include <ostream>

#if defined(NOA_COMPILER_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <yaml-cpp/yaml.h>

#if defined(NOA_COMPILER_CLANG)
#pragma GCC diagnostic pop
#endif

namespace YAML {
    template<>
    struct convert<noa::size4_t> {
        static Node encode(const noa::size4_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.push_back(rhs[3]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::size4_t& rhs) {
            if (!node.IsSequence() || node.size() != 4)
                return false;
            rhs[0] = node[0].as<size_t>();
            rhs[1] = node[1].as<size_t>();
            rhs[2] = node[2].as<size_t>();
            rhs[3] = node[3].as<size_t>();
            return true;
        }
    };

    template<>
    struct convert<noa::size3_t> {
        static Node encode(const noa::size3_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::size3_t& rhs) {
            if (!node.IsSequence() || node.size() != 3)
                return false;
            rhs[0] = node[0].as<size_t>();
            rhs[1] = node[1].as<size_t>();
            rhs[2] = node[2].as<size_t>();
            return true;
        }
    };

    template<>
    struct convert<noa::size2_t> {
        static Node encode(const noa::size2_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::size2_t& rhs) {
            if (!node.IsSequence() || node.size() != 2)
                return false;
            rhs[0] = node[0].as<size_t>();
            rhs[1] = node[1].as<size_t>();
            return true;
        }
    };

    template<>
    struct convert<noa::int4_t> {
        static Node encode(const noa::int4_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.push_back(rhs[3]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::int4_t& rhs) {
            if (!node.IsSequence() || node.size() != 4)
                return false;
            rhs[0] = node[0].as<int>();
            rhs[1] = node[1].as<int>();
            rhs[2] = node[2].as<int>();
            rhs[3] = node[3].as<int>();
            return true;
        }
    };

    template<>
    struct convert<noa::int3_t> {
        static Node encode(const noa::int3_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::int3_t& rhs) {
            if (!node.IsSequence() || node.size() != 3)
                return false;
            rhs[0] = node[0].as<int>();
            rhs[1] = node[1].as<int>();
            rhs[2] = node[2].as<int>();
            return true;
        }
    };

    template<>
    struct convert<noa::uint4_t> {
        static Node encode(const noa::uint4_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.push_back(rhs[3]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::uint4_t& rhs) {
            if (!node.IsSequence() || node.size() != 4)
                return false;
            rhs[0] = node[0].as<uint>();
            rhs[1] = node[1].as<uint>();
            rhs[2] = node[2].as<uint>();
            rhs[3] = node[3].as<uint>();
            return true;
        }
    };

    template<>
    struct convert<noa::uint3_t> {
        static Node encode(const noa::uint3_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::uint3_t& rhs) {
            if (!node.IsSequence() || node.size() != 3)
                return false;
            rhs[0] = node[0].as<uint>();
            rhs[1] = node[1].as<uint>();
            rhs[2] = node[2].as<uint>();
            return true;
        }
    };


    template<>
    struct convert<noa::float4_t> {
        static Node encode(const noa::float4_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.push_back(rhs[3]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::float4_t& rhs) {
            if (!node.IsSequence() || node.size() != 4)
                return false;
            rhs[0] = node[0].as<float>();
            rhs[1] = node[1].as<float>();
            rhs[2] = node[2].as<float>();
            rhs[3] = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<noa::float3_t> {
        static Node encode(const noa::float3_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.push_back(rhs[2]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::float3_t& rhs) {
            if (!node.IsSequence() || node.size() != 3)
                return false;
            rhs[0] = node[0].as<float>();
            rhs[1] = node[1].as<float>();
            rhs[2] = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<noa::float2_t> {
        static Node encode(const noa::float2_t& rhs) {
            Node node;
            node.push_back(rhs[0]);
            node.push_back(rhs[1]);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, noa::float2_t& rhs) {
            if (!node.IsSequence() || node.size() != 2)
                return false;
            rhs[0] = node[0].as<float>();
            rhs[1] = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<noa::path_t> {
        static Node encode(const noa::path_t& rhs) {
            return convert<std::string>::encode(rhs.string());
        }

        static bool decode(const Node& node, noa::path_t& rhs) {
            std::string str;
            bool status = convert<std::string>::decode(node, str);
            rhs = str;
            return status;
        }
    };

    template<>
    struct convert<noa::InterpMode> {
        static Node encode(const noa::InterpMode& rhs) {
            std::ostringstream stream;
            stream << rhs;
            return convert<std::string>::encode(stream.str());
        }

        static bool decode(const Node& node, noa::InterpMode& rhs) {
            if (!node.IsScalar())
                return false;
            const std::string& buffer = node.Scalar();

            using namespace ::noa;
            if (buffer == "INTERP_NEAREST")
                rhs = INTERP_NEAREST;
            else if (buffer == "INTERP_LINEAR")
                rhs = INTERP_LINEAR;
            else if (buffer == "INTERP_COSINE")
                rhs = INTERP_COSINE;
            else if (buffer == "INTERP_CUBIC")
                rhs = INTERP_CUBIC;
            else if (buffer == "INTERP_CUBIC_BSPLINE")
                rhs = INTERP_CUBIC_BSPLINE;
            else if (buffer == "INTERP_LINEAR_FAST")
                rhs = INTERP_LINEAR_FAST;
            else if (buffer == "INTERP_COSINE_FAST")
                rhs = INTERP_COSINE_FAST;
            else if (buffer == "INTERP_CUBIC_BSPLINE_FAST")
                rhs = INTERP_CUBIC_BSPLINE_FAST;
            else
                return false;
            return true;
        }
    };

    template<>
    struct convert<noa::BorderMode> {
        static Node encode(const noa::BorderMode& rhs) {
            std::ostringstream stream;
            stream << rhs;
            return convert<std::string>::encode(stream.str());
        }

        static bool decode(const Node& node, noa::BorderMode& rhs) {
            if (!node.IsScalar())
                return false;
            const std::string& buffer = node.Scalar();

            using namespace ::noa;
            if (buffer == "BORDER_NOTHING")
                rhs = BORDER_NOTHING;
            else if (buffer == "BORDER_ZERO")
                rhs = BORDER_ZERO;
            else if (buffer == "BORDER_VALUE")
                rhs = BORDER_VALUE;
            else if (buffer == "BORDER_CLAMP")
                rhs = BORDER_CLAMP;
            else if (buffer == "BORDER_REFLECT")
                rhs = BORDER_REFLECT;
            else if (buffer == "BORDER_MIRROR")
                rhs = BORDER_MIRROR;
            else if (buffer == "BORDER_PERIODIC")
                rhs = BORDER_PERIODIC;
            else
                return false;
            return true;
        }
    };

    template<>
    struct convert<noa::fft::Remap> {
        static Node encode(const noa::fft::Remap& rhs) {
            std::ostringstream stream;
            stream << rhs;
            return convert<std::string>::encode(stream.str());
        }

        static bool decode(const Node& node, noa::fft::Remap& rhs) {
            if (!node.IsScalar())
                return false;
            const std::string& buffer = node.Scalar();

            using namespace ::noa;
            if (buffer == "H2H")
                rhs = fft::H2H;
            else if (buffer == "HC2HC")
                rhs = fft::HC2HC;
            else if (buffer == "H2HC")
                rhs = fft::H2HC;
            else if (buffer == "HC2H")
                rhs = fft::HC2H;
            else if (buffer == "H2F")
                rhs = fft::H2F;
            else if (buffer == "F2H")
                rhs = fft::F2H;
            else if (buffer == "F2FC")
                rhs = fft::F2FC;
            else if (buffer == "FC2F")
                rhs = fft::FC2F;
            else if (buffer == "HC2F")
                rhs = fft::HC2F;
            else if (buffer == "F2HC")
                rhs = fft::F2HC;
            else if (buffer == "H2FC")
                rhs = fft::H2FC;
            else if (buffer == "FC2H")
                rhs = fft::FC2H;
            else if (buffer == "F2F")
                rhs = fft::F2F;
            else if (buffer == "FC2FC")
                rhs = fft::FC2FC;
            else
                return false;
            return true;
        }
    };
}
