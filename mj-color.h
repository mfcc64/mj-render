/*
 * Copyright (C) 2020 Muhammad Faiz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MJ_COLOR_H
#define MJ_COLOR_H 1

#include <stdio.h>

struct MJ_Color {
    float v[4];

    inline void is_valid() const
    {
        for (int i = 0; i < 3; i++)
            if (!(v[i] >= 0.0f && v[i] <= 1.0f))
                throw "invalid color";
    }
};

MJ_Color mj_color_average(const MJ_Color color[], float status, int count)
{
    MJ_Color result = {{ 0, 0, 0, status }};
    for (int k = 0; k < count; k++)
        for (int i = 0; i < 3; i++)
            result.v[i] += color[k].v[i];

    for (int i = 0; i < 3; i++)
        result.v[i] /= count;

    return result;
}

class MJ_ColorPalette {
public:
    MJ_ColorPalette(const char *filename = NULL, double offset = 0.0)
    {
        m_offset = offset;

        if (!filename) {
            m_color = m_default_color;
            m_nb_color = m_nb_default_color;
            m_infinity_color = (MJ_Color){};
            m_gen_grad();
            return;
        }

        FILE *fp = fopen(filename, "rb");
        if (!fp)
            throw "cannot open palette file";

        try {
            if (fscanf(fp, "%f %f %f", &m_infinity_color.v[0], &m_infinity_color.v[1], &m_infinity_color.v[2]) != 3)
                throw "invalid palette file";
            m_infinity_color.is_valid();

            if (fscanf(fp, "%d", &m_nb_color) != 1)
                throw "invalid palette file";
            if (m_nb_color < 1 || m_nb_color > 65536)
                throw "invalid palette file";

            m_color = new MJ_Color[m_nb_color];
            for (int k = 0; k < m_nb_color; k++) {
                if (fscanf(fp, "%f %f %f", (float *)&m_color[k].v[0], (float *)&m_color[k].v[1], (float *)&m_color[k].v[2]) != 3)
                    throw "invalid palette file";
                m_color[k].is_valid();
            }

            char tail;
            if (fscanf(fp, " %c", &tail) == 1)
                throw "invalid palette file";
            m_gen_grad();
        } catch (...) {
            fclose(fp);
            throw;
        }

        fclose(fp);
    }

    ~MJ_ColorPalette()
    {
        if (m_color && m_color != m_default_color)
            delete[] m_color;
        if (m_grad)
            delete[] m_grad;
    }

    MJ_Color color(double x, float status) const
    {
        x += m_offset;
        x = x - floor(x);
        x *= m_nb_color;
        int m = floor(x);
        int n = (m + 1) % m_nb_color;
        float f = x - m;
        MJ_Color result = {{ 0, 0, 0, status }};

        for (int k = 0; k < 3; k++) {
            float d = m_color[m].v[k];
            float c = m_grad[m].v[k];
            float b = 3.0f * m_color[n].v[k] - m_grad[n].v[k] - 2.0f * c - 3.0f * d;
            float a = m_color[n].v[k] - b - c - d;
            result.v[k] = ((((a * f) + b) * f) + c) * f + d;
        }

        return result;
    }

    MJ_Color infinity_color(float status) const
    {
        MJ_Color result = m_infinity_color;
        result.v[3] = status;
        return result;
    }

private:
    MJ_Color m_infinity_color;
    const MJ_Color *m_color;
    MJ_Color *m_grad;
    double   m_offset;
    int      m_nb_color;

    static const MJ_Color m_default_color[];
    static const int m_nb_default_color;

    void m_gen_grad()
    {
        m_grad = new MJ_Color[m_nb_color];
        for (int k = 0; k < m_nb_color; k++) {
            int prev = (k + m_nb_color - 1) % m_nb_color;
            int next = (k + 1) % m_nb_color;

            for (int c = 0; c < 3; c++) {
                float g_prev = m_color[k].v[c] - m_color[prev].v[c];
                float g_next = m_color[next].v[c] - m_color[k].v[c];
                float g_sum = fabsf(g_prev) + fabsf(g_next);
                m_grad[k].v[c] = g_sum > 0.0f ? (fabsf(g_next) * g_prev + fabsf(g_prev) * g_next) / g_sum : 0.0f;
            }
        }
    }

};

const MJ_Color MJ_ColorPalette::m_default_color[] = {
    {{ 0.000000, 0.027451, 0.392157 }},
    {{ 0.000104, 0.028892, 0.402546 }},
    {{ 0.000414, 0.031159, 0.413131 }},
    {{ 0.000926, 0.034218, 0.423892 }},
    {{ 0.001634, 0.038038, 0.434815 }},
    {{ 0.002534, 0.042587, 0.445881 }},
    {{ 0.003623, 0.047832, 0.457074 }},
    {{ 0.004895, 0.053742, 0.468376 }},
    {{ 0.006346, 0.060283, 0.479771 }},
    {{ 0.007972, 0.067424, 0.491241 }},
    {{ 0.009768, 0.075133, 0.502770 }},
    {{ 0.011729, 0.083378, 0.514340 }},
    {{ 0.013853, 0.092125, 0.525935 }},
    {{ 0.016133, 0.101344, 0.537537 }},
    {{ 0.018565, 0.111001, 0.549129 }},
    {{ 0.021146, 0.121065, 0.560695 }},
    {{ 0.023870, 0.131504, 0.572217 }},
    {{ 0.026734, 0.142284, 0.583679 }},
    {{ 0.029732, 0.153375, 0.595062 }},
    {{ 0.032860, 0.164744, 0.606351 }},
    {{ 0.036115, 0.176358, 0.617528 }},
    {{ 0.039491, 0.188185, 0.628576 }},
    {{ 0.042984, 0.200194, 0.639479 }},
    {{ 0.046589, 0.212352, 0.650218 }},
    {{ 0.050303, 0.224627, 0.660778 }},
    {{ 0.054120, 0.236986, 0.671140 }},
    {{ 0.058037, 0.249398, 0.681289 }},
    {{ 0.062048, 0.261830, 0.691207 }},
    {{ 0.066150, 0.274250, 0.700876 }},
    {{ 0.070338, 0.286626, 0.710281 }},
    {{ 0.074607, 0.298926, 0.719404 }},
    {{ 0.078954, 0.311117, 0.728227 }},
    {{ 0.083373, 0.323167, 0.736734 }},
    {{ 0.087861, 0.335044, 0.744909 }},
    {{ 0.092412, 0.346716, 0.752733 }},
    {{ 0.097023, 0.358151, 0.760189 }},
    {{ 0.101688, 0.369316, 0.767262 }},
    {{ 0.106405, 0.380180, 0.773933 }},
    {{ 0.111167, 0.390709, 0.780186 }},
    {{ 0.115971, 0.400873, 0.786004 }},
    {{ 0.120812, 0.410638, 0.791369 }},
    {{ 0.125686, 0.419973, 0.796265 }},
    {{ 0.130962, 0.429226, 0.800938 }},
    {{ 0.136942, 0.438705, 0.805603 }},
    {{ 0.143602, 0.448399, 0.810259 }},
    {{ 0.150920, 0.458296, 0.814905 }},
    {{ 0.158872, 0.468385, 0.819538 }},
    {{ 0.167435, 0.478655, 0.824156 }},
    {{ 0.176587, 0.489094, 0.828758 }},
    {{ 0.186304, 0.499692, 0.833341 }},
    {{ 0.196563, 0.510436, 0.837903 }},
    {{ 0.207341, 0.521317, 0.842443 }},
    {{ 0.218615, 0.532322, 0.846959 }},
    {{ 0.230362, 0.543440, 0.851447 }},
    {{ 0.242559, 0.554660, 0.855908 }},
    {{ 0.255183, 0.565971, 0.860338 }},
    {{ 0.268211, 0.577361, 0.864735 }},
    {{ 0.281620, 0.588820, 0.869099 }},
    {{ 0.295386, 0.600336, 0.873426 }},
    {{ 0.309487, 0.611897, 0.877714 }},
    {{ 0.323900, 0.623493, 0.881963 }},
    {{ 0.338601, 0.635112, 0.886169 }},
    {{ 0.353568, 0.646742, 0.890331 }},
    {{ 0.368778, 0.658374, 0.894446 }},
    {{ 0.384206, 0.669995, 0.898514 }},
    {{ 0.399832, 0.681594, 0.902532 }},
    {{ 0.415630, 0.693160, 0.906497 }},
    {{ 0.431579, 0.704682, 0.910409 }},
    {{ 0.447655, 0.716148, 0.914264 }},
    {{ 0.463836, 0.727547, 0.918062 }},
    {{ 0.480097, 0.738868, 0.921799 }},
    {{ 0.496416, 0.750099, 0.925475 }},
    {{ 0.512771, 0.761230, 0.929087 }},
    {{ 0.529137, 0.772250, 0.932633 }},
    {{ 0.545493, 0.783145, 0.936111 }},
    {{ 0.561814, 0.793907, 0.939519 }},
    {{ 0.578078, 0.804523, 0.942856 }},
    {{ 0.594262, 0.814982, 0.946119 }},
    {{ 0.610343, 0.825272, 0.949306 }},
    {{ 0.626297, 0.835384, 0.952416 }},
    {{ 0.642102, 0.845304, 0.955446 }},
    {{ 0.657734, 0.855023, 0.958395 }},
    {{ 0.673171, 0.864528, 0.961260 }},
    {{ 0.688390, 0.873809, 0.964039 }},
    {{ 0.703366, 0.882854, 0.966731 }},
    {{ 0.718079, 0.891652, 0.969334 }},
    {{ 0.732503, 0.900192, 0.971845 }},
    {{ 0.746617, 0.908462, 0.974263 }},
    {{ 0.760397, 0.916451, 0.976585 }},
    {{ 0.773820, 0.924149, 0.978810 }},
    {{ 0.786863, 0.931543, 0.980936 }},
    {{ 0.799503, 0.938623, 0.982961 }},
    {{ 0.811717, 0.945376, 0.984882 }},
    {{ 0.823483, 0.951793, 0.986699 }},
    {{ 0.834776, 0.957862, 0.988408 }},
    {{ 0.845573, 0.963570, 0.990008 }},
    {{ 0.855853, 0.968908, 0.991497 }},
    {{ 0.865591, 0.973864, 0.992873 }},
    {{ 0.874766, 0.978427, 0.994134 }},
    {{ 0.883352, 0.982585, 0.995277 }},
    {{ 0.891329, 0.986327, 0.996302 }},
    {{ 0.898671, 0.989642, 0.997207 }},
    {{ 0.905358, 0.992518, 0.997988 }},
    {{ 0.911364, 0.994945, 0.998644 }},
    {{ 0.916668, 0.996911, 0.999174 }},
    {{ 0.921247, 0.998405, 0.999574 }},
    {{ 0.925076, 0.999415, 0.999844 }},
    {{ 0.928134, 0.999931, 0.999982 }},
    {{ 0.930488, 0.999961, 0.999788 }},
    {{ 0.932709, 0.999631, 0.998010 }},
    {{ 0.934901, 0.998971, 0.994478 }},
    {{ 0.937066, 0.997989, 0.989258 }},
    {{ 0.939202, 0.996690, 0.982415 }},
    {{ 0.941308, 0.995083, 0.974013 }},
    {{ 0.943385, 0.993175, 0.964118 }},
    {{ 0.945432, 0.990972, 0.952794 }},
    {{ 0.947448, 0.988482, 0.940107 }},
    {{ 0.949434, 0.985712, 0.926121 }},
    {{ 0.951388, 0.982669, 0.910901 }},
    {{ 0.953310, 0.979360, 0.894513 }},
    {{ 0.955200, 0.975792, 0.877020 }},
    {{ 0.957057, 0.971972, 0.858489 }},
    {{ 0.958881, 0.967908, 0.838983 }},
    {{ 0.960672, 0.963606, 0.818569 }},
    {{ 0.962429, 0.959074, 0.797310 }},
    {{ 0.964152, 0.954318, 0.775272 }},
    {{ 0.965839, 0.949347, 0.752520 }},
    {{ 0.967492, 0.944166, 0.729119 }},
    {{ 0.969109, 0.938783, 0.705133 }},
    {{ 0.970690, 0.933206, 0.680628 }},
    {{ 0.972234, 0.927440, 0.655668 }},
    {{ 0.973742, 0.921494, 0.630319 }},
    {{ 0.975212, 0.915375, 0.604644 }},
    {{ 0.976644, 0.909089, 0.578711 }},
    {{ 0.978039, 0.902643, 0.552582 }},
    {{ 0.979394, 0.896046, 0.526323 }},
    {{ 0.980711, 0.889303, 0.500000 }},
    {{ 0.981988, 0.882423, 0.473677 }},
    {{ 0.983225, 0.875411, 0.447418 }},
    {{ 0.984422, 0.868276, 0.421289 }},
    {{ 0.985578, 0.861023, 0.395356 }},
    {{ 0.986693, 0.853662, 0.369681 }},
    {{ 0.987767, 0.846198, 0.344332 }},
    {{ 0.988798, 0.838638, 0.319372 }},
    {{ 0.989787, 0.830991, 0.294867 }},
    {{ 0.990733, 0.823262, 0.270881 }},
    {{ 0.991636, 0.815459, 0.247480 }},
    {{ 0.992495, 0.807589, 0.224728 }},
    {{ 0.993310, 0.799659, 0.202690 }},
    {{ 0.994080, 0.791677, 0.181431 }},
    {{ 0.994805, 0.783649, 0.161017 }},
    {{ 0.995485, 0.775582, 0.141511 }},
    {{ 0.996119, 0.767484, 0.122980 }},
    {{ 0.996707, 0.759362, 0.105487 }},
    {{ 0.997248, 0.751223, 0.089099 }},
    {{ 0.997741, 0.743073, 0.073879 }},
    {{ 0.998188, 0.734921, 0.059893 }},
    {{ 0.998586, 0.726772, 0.047206 }},
    {{ 0.998936, 0.718635, 0.035882 }},
    {{ 0.999237, 0.710517, 0.025987 }},
    {{ 0.999488, 0.702423, 0.017585 }},
    {{ 0.999690, 0.694363, 0.010742 }},
    {{ 0.999842, 0.686342, 0.005522 }},
    {{ 0.999944, 0.678368, 0.001990 }},
    {{ 0.999994, 0.670447, 0.000212 }},
    {{ 0.999734, 0.662481, 0.000000 }},
    {{ 0.997754, 0.653889, 0.000000 }},
    {{ 0.993903, 0.644613, 0.000000 }},
    {{ 0.988253, 0.634683, 0.000000 }},
    {{ 0.980876, 0.624132, 0.000000 }},
    {{ 0.971843, 0.612991, 0.000000 }},
    {{ 0.961227, 0.601292, 0.000000 }},
    {{ 0.949099, 0.589068, 0.000000 }},
    {{ 0.935533, 0.576349, 0.000000 }},
    {{ 0.920598, 0.563168, 0.000000 }},
    {{ 0.904369, 0.549557, 0.000000 }},
    {{ 0.886916, 0.535547, 0.000000 }},
    {{ 0.868311, 0.521170, 0.000000 }},
    {{ 0.848627, 0.506459, 0.000000 }},
    {{ 0.827935, 0.491444, 0.000000 }},
    {{ 0.806308, 0.476159, 0.000000 }},
    {{ 0.783817, 0.460634, 0.000000 }},
    {{ 0.760534, 0.444901, 0.000000 }},
    {{ 0.736532, 0.428993, 0.000000 }},
    {{ 0.711882, 0.412942, 0.000000 }},
    {{ 0.686656, 0.396778, 0.000000 }},
    {{ 0.660927, 0.380534, 0.000000 }},
    {{ 0.634765, 0.364242, 0.000000 }},
    {{ 0.608244, 0.347933, 0.000000 }},
    {{ 0.581435, 0.331640, 0.000000 }},
    {{ 0.554410, 0.315394, 0.000000 }},
    {{ 0.527241, 0.299227, 0.000000 }},
    {{ 0.500000, 0.283172, 0.000000 }},
    {{ 0.472759, 0.267259, 0.000000 }},
    {{ 0.445590, 0.251521, 0.000000 }},
    {{ 0.418565, 0.235989, 0.000000 }},
    {{ 0.391756, 0.220695, 0.000000 }},
    {{ 0.365235, 0.205672, 0.000000 }},
    {{ 0.339073, 0.190951, 0.000000 }},
    {{ 0.313344, 0.176564, 0.000000 }},
    {{ 0.288118, 0.162543, 0.000000 }},
    {{ 0.263468, 0.148919, 0.000000 }},
    {{ 0.239466, 0.135724, 0.000000 }},
    {{ 0.216183, 0.122991, 0.000000 }},
    {{ 0.193692, 0.110751, 0.000000 }},
    {{ 0.172065, 0.099036, 0.000000 }},
    {{ 0.151373, 0.087878, 0.000000 }},
    {{ 0.131689, 0.077308, 0.000000 }},
    {{ 0.113084, 0.067359, 0.000000 }},
    {{ 0.095631, 0.058062, 0.000000 }},
    {{ 0.079402, 0.049450, 0.000000 }},
    {{ 0.064467, 0.041553, 0.000000 }},
    {{ 0.050901, 0.034405, 0.000000 }},
    {{ 0.038773, 0.028036, 0.000000 }},
    {{ 0.028157, 0.022478, 0.000000 }},
    {{ 0.019124, 0.017764, 0.000000 }},
    {{ 0.011747, 0.013926, 0.000000 }},
    {{ 0.006097, 0.010994, 0.000000 }},
    {{ 0.002246, 0.009002, 0.000000 }},
    {{ 0.000266, 0.007980, 0.000000 }},
    {{ 0.000000, 0.007847, 0.000138 }},
    {{ 0.000000, 0.007879, 0.001291 }},
    {{ 0.000000, 0.007943, 0.003574 }},
    {{ 0.000000, 0.008039, 0.006936 }},
    {{ 0.000000, 0.008166, 0.011326 }},
    {{ 0.000000, 0.008326, 0.016694 }},
    {{ 0.000000, 0.008516, 0.022988 }},
    {{ 0.000000, 0.008737, 0.030159 }},
    {{ 0.000000, 0.008989, 0.038156 }},
    {{ 0.000000, 0.009271, 0.046929 }},
    {{ 0.000000, 0.009584, 0.056426 }},
    {{ 0.000000, 0.009926, 0.066598 }},
    {{ 0.000000, 0.010298, 0.077393 }},
    {{ 0.000000, 0.010700, 0.088762 }},
    {{ 0.000000, 0.011130, 0.100653 }},
    {{ 0.000000, 0.011590, 0.113016 }},
    {{ 0.000000, 0.012078, 0.125801 }},
    {{ 0.000000, 0.012594, 0.138957 }},
    {{ 0.000000, 0.013139, 0.152433 }},
    {{ 0.000000, 0.013711, 0.166179 }},
    {{ 0.000000, 0.014311, 0.180144 }},
    {{ 0.000000, 0.014938, 0.194278 }},
    {{ 0.000000, 0.015593, 0.208530 }},
    {{ 0.000000, 0.016274, 0.222850 }},
    {{ 0.000000, 0.016982, 0.237187 }},
    {{ 0.000000, 0.017716, 0.251490 }},
    {{ 0.000000, 0.018476, 0.265709 }},
    {{ 0.000000, 0.019261, 0.279794 }},
    {{ 0.000000, 0.020073, 0.293694 }},
    {{ 0.000000, 0.020909, 0.307357 }},
    {{ 0.000000, 0.021771, 0.320735 }},
    {{ 0.000000, 0.022657, 0.333776 }},
    {{ 0.000000, 0.023568, 0.346429 }},
    {{ 0.000000, 0.024503, 0.358644 }},
    {{ 0.000000, 0.025462, 0.370371 }},
    {{ 0.000000, 0.026445, 0.381559 }}
};

const int MJ_ColorPalette::m_nb_default_color = sizeof(m_default_color) / sizeof(m_default_color[0]);

#endif
