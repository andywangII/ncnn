// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

static void conv3x3s1_winograd64_transform_kernel_pack4_neon(const Mat& kernel, Mat& kernel_tm_pack4, int inch, int outch)
{
    // winograd23 transform kernel
    Mat kernel_tm;
    kernel_tm.create(8*8, inch, outch);

    const float ktm[8][3] = {
        {   1.0f,     0.0f,     0.0f},
        {-2.0f/9,  -2.0f/9,  -2.0f/9},
        {-2.0f/9,   2.0f/9,  -2.0f/9},
        {1.0f/90,  1.0f/45,  2.0f/45},
        {1.0f/90, -1.0f/45,  2.0f/45},
        {1.0f/45,  1.0f/90, 1.0f/180},
        {1.0f/45, -1.0f/90, 1.0f/180},
        {   0.0f,     0.0f,     1.0f}
    };

    #pragma omp parallel for
    for (int p = 0; p<outch; p++)
    {
        for (int q = 0; q<inch; q++)
        {
            const float* kernel0 = (const float*)kernel + p*inch * 9 + q * 9;
            float* kernel_tm0 = kernel_tm.channel(p).row(q);

            // transform kernel, transposed
            const float* k0 = kernel0;
            const float* k1 = kernel0 + 3;
            const float* k2 = kernel0 + 6;

            // h
            float tmp[8][3];
            for (int i=0; i<8; i++)
            {
                tmp[i][0] = k0[0] * ktm[i][0] + k0[1] * ktm[i][1] + k0[2] * ktm[i][2];
                tmp[i][1] = k1[0] * ktm[i][0] + k1[1] * ktm[i][1] + k1[2] * ktm[i][2];
                tmp[i][2] = k2[0] * ktm[i][0] + k2[1] * ktm[i][1] + k2[2] * ktm[i][2];
            }

            // v
            for (int j=0; j<8; j++)
            {
                float* tmpp = &tmp[j][0];

                for (int i=0; i<8; i++)
                {
                    kernel_tm0[j*8 + i] = tmpp[0] * ktm[i][0] + tmpp[1] * ktm[i][1] + tmpp[2] * ktm[i][2];
                }
            }
        }
    }

    // interleave
    // src = 64-inch-outch
    // dst = 4b-4a-inch/4a-64-outch/4b;
#if __aarch64__
    kernel_tm_pack4.create(2 * inch/4, 64, (outch/4)/2 + (outch/4)%2, (size_t)4u*16, 16);
#else
    kernel_tm_pack4.create(inch/4, 64, outch/4, (size_t)4u*16, 16);
#endif

    int q=0;
#if __aarch64__
    for (; q+7<outch; q+=8)
    {
        const Mat k0 = kernel_tm.channel(q);
        const Mat k1 = kernel_tm.channel(q+1);
        const Mat k2 = kernel_tm.channel(q+2);
        const Mat k3 = kernel_tm.channel(q+3);
        const Mat k4 = kernel_tm.channel(q+4);
        const Mat k5 = kernel_tm.channel(q+5);
        const Mat k6 = kernel_tm.channel(q+6);
        const Mat k7 = kernel_tm.channel(q+7);

        Mat g0 = kernel_tm_pack4.channel(q/8);

        for (int k=0; k<64; k++)
        {
            float* g00 = g0.row(k);

            for (int p=0; p+3<inch; p+=4)
            {
                const float* k00 = k0.row(p);
                const float* k01 = k0.row(p+1);
                const float* k02 = k0.row(p+2);
                const float* k03 = k0.row(p+3);

                const float* k10 = k1.row(p);
                const float* k11 = k1.row(p+1);
                const float* k12 = k1.row(p+2);
                const float* k13 = k1.row(p+3);

                const float* k20 = k2.row(p);
                const float* k21 = k2.row(p+1);
                const float* k22 = k2.row(p+2);
                const float* k23 = k2.row(p+3);

                const float* k30 = k3.row(p);
                const float* k31 = k3.row(p+1);
                const float* k32 = k3.row(p+2);
                const float* k33 = k3.row(p+3);

                const float* k40 = k4.row(p);
                const float* k41 = k4.row(p+1);
                const float* k42 = k4.row(p+2);
                const float* k43 = k4.row(p+3);

                const float* k50 = k5.row(p);
                const float* k51 = k5.row(p+1);
                const float* k52 = k5.row(p+2);
                const float* k53 = k5.row(p+3);

                const float* k60 = k6.row(p);
                const float* k61 = k6.row(p+1);
                const float* k62 = k6.row(p+2);
                const float* k63 = k6.row(p+3);

                const float* k70 = k7.row(p);
                const float* k71 = k7.row(p+1);
                const float* k72 = k7.row(p+2);
                const float* k73 = k7.row(p+3);

                g00[0] = k00[k];
                g00[1] = k10[k];
                g00[2] = k20[k];
                g00[3] = k30[k];

                g00[4] = k40[k];
                g00[5] = k50[k];
                g00[6] = k60[k];
                g00[7] = k70[k];

                g00[8] = k01[k];
                g00[9] = k11[k];
                g00[10] = k21[k];
                g00[11] = k31[k];

                g00[12] = k41[k];
                g00[13] = k51[k];
                g00[14] = k61[k];
                g00[15] = k71[k];

                g00[16] = k02[k];
                g00[17] = k12[k];
                g00[18] = k22[k];
                g00[19] = k32[k];

                g00[20] = k42[k];
                g00[21] = k52[k];
                g00[22] = k62[k];
                g00[23] = k72[k];

                g00[24] = k03[k];
                g00[25] = k13[k];
                g00[26] = k23[k];
                g00[27] = k33[k];

                g00[28] = k43[k];
                g00[29] = k53[k];
                g00[30] = k63[k];
                g00[31] = k73[k];

                g00 += 32;
            }
        }
    }
#endif // __aarch64__
    for (; q+3<outch; q+=4)
    {
        const Mat k0 = kernel_tm.channel(q);
        const Mat k1 = kernel_tm.channel(q+1);
        const Mat k2 = kernel_tm.channel(q+2);
        const Mat k3 = kernel_tm.channel(q+3);

#if __aarch64__
        Mat g0 = kernel_tm_pack4.channel(q/8+(q%8)/4);
#else
        Mat g0 = kernel_tm_pack4.channel(q/4);
#endif

        for (int k=0; k<64; k++)
        {
            float* g00 = g0.row(k);

            for (int p=0; p+3<inch; p+=4)
            {
                const float* k00 = k0.row(p);
                const float* k01 = k0.row(p+1);
                const float* k02 = k0.row(p+2);
                const float* k03 = k0.row(p+3);

                const float* k10 = k1.row(p);
                const float* k11 = k1.row(p+1);
                const float* k12 = k1.row(p+2);
                const float* k13 = k1.row(p+3);

                const float* k20 = k2.row(p);
                const float* k21 = k2.row(p+1);
                const float* k22 = k2.row(p+2);
                const float* k23 = k2.row(p+3);

                const float* k30 = k3.row(p);
                const float* k31 = k3.row(p+1);
                const float* k32 = k3.row(p+2);
                const float* k33 = k3.row(p+3);

                g00[0] = k00[k];
                g00[1] = k10[k];
                g00[2] = k20[k];
                g00[3] = k30[k];

                g00[4] = k01[k];
                g00[5] = k11[k];
                g00[6] = k21[k];
                g00[7] = k31[k];

                g00[8] = k02[k];
                g00[9] = k12[k];
                g00[10] = k22[k];
                g00[11] = k32[k];

                g00[12] = k03[k];
                g00[13] = k13[k];
                g00[14] = k23[k];
                g00[15] = k33[k];

                g00 += 16;
            }
        }
    }
}

static void conv3x3s1_winograd64_pack4_neon(const Mat& bottom_blob, Mat& top_blob, const Mat& kernel_tm, const Mat& _bias, const Option& opt)
{
    int w = bottom_blob.w;
    int h = bottom_blob.h;
    int inch = bottom_blob.c;
    size_t elemsize = bottom_blob.elemsize;
    int elempack = bottom_blob.elempack;

    int outw = top_blob.w;
    int outh = top_blob.h;
    int outch = top_blob.c;

    // pad to 6n+2
    Mat bottom_blob_bordered = bottom_blob;

    outw = (outw + 5) / 6 * 6;
    outh = (outh + 5) / 6 * 6;

    w = outw + 2;
    h = outh + 2;
    copy_make_border(bottom_blob, bottom_blob_bordered, 0, h - bottom_blob.h, 0, w - bottom_blob.w, BORDER_CONSTANT, 0.f, opt);

    const float* bias = _bias;

    // BEGIN transform input
    Mat bottom_blob_tm;
    {
        int w_tm = outw / 6 * 8;
        int h_tm = outh / 6 * 8;

        const int tiles = w_tm/8 * h_tm/8;

        bottom_blob_tm.create(tiles, 64, inch, elemsize, elempack, opt.workspace_allocator);

//         const float itm[8][8] = {
//             {1.0f,  0.0f, -5.25f,  0.00f,  5.25f,  0.00f, -1.0f, 0.0f},
//
//             {0.0f,  1.0f,  1.00f, -4.25f, -4.25f,  1.00f,  1.0f, 0.0f},
//             {0.0f, -1.0f,  1.00f,  4.25f, -4.25f, -1.00f,  1.0f, 0.0f},
//
//             {0.0f,  0.5f,  0.25f, -2.50f, -1.25f,  2.00f,  1.0f, 0.0f},
//             {0.0f, -0.5f,  0.25f,  2.50f, -1.25f, -2.00f,  1.0f, 0.0f},
//
//             {0.0f,  2.0f,  4.00f, -2.50f, -5.00f,  0.50f,  1.0f, 0.0f},
//             {0.0f, -2.0f,  4.00f,  2.50f, -5.00f, -0.50f,  1.0f, 0.0f},
//
//             {0.0f, -1.0f,  0.00f,  5.25f,  0.00f, -5.25f,  0.0f, 1.0f}
//         };

        // 0 = r00 - r06 + (r04 - r02) * 5.25
        // 7 = r07 - r01 + (r03 - r05) * 5.25

        // 1 = (r02 + r06 - r04 * 4.25) + (r01 - r03 * 4.25 + r05)
        // 2 = (r02 + r06 - r04 * 4.25) - (r01 - r03 * 4.25 + r05)

        // 3 = (r06 + r02 * 0.25 - r04 * 1.25) + (r01 * 0.5 - r03 * 2.5 + r05 * 2)
        // 4 = (r06 + r02 * 0.25 - r04 * 1.25) - (r01 * 0.5 - r03 * 2.5 + r05 * 2)

        // reuse r04 * 1.25
        // reuse r03 * 2.5
        // 5 = (r06 + (r02 - r04 * 1.25) * 4) + (r01 * 2 - r03 * 2.5 + r05 * 0.5)
        // 6 = (r06 + (r02 - r04 * 1.25) * 4) - (r01 * 2 - r03 * 2.5 + r05 * 0.5)

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int q = 0; q<inch; q++)
        {
            const Mat img0 = bottom_blob_bordered.channel(q);
            Mat img0_tm = bottom_blob_tm.channel(q);

            float tmp[8][8][4];

            // tile
            for (int i=0; i<h_tm/8; i++)
            {
                for (int j=0; j<w_tm/8; j++)
                {
                    const float* r0 = img0.row(i * 6) + (j * 6) * 4;

                    for (int m=0; m<8; m++)
                    {
                        float32x4_t _r00 = vld1q_f32(r0);
                        float32x4_t _r01 = vld1q_f32(r0 + 4);
                        float32x4_t _r02 = vld1q_f32(r0 + 8);
                        float32x4_t _r03 = vld1q_f32(r0 + 12);
                        float32x4_t _r04 = vld1q_f32(r0 + 16);
                        float32x4_t _r05 = vld1q_f32(r0 + 20);
                        float32x4_t _r06 = vld1q_f32(r0 + 24);
                        float32x4_t _r07 = vld1q_f32(r0 + 28);

                        float32x4_t _tmp0m = vmlaq_n_f32(vsubq_f32(_r00, _r06), vsubq_f32(_r04, _r02), 5.25f);
                        float32x4_t _tmp7m = vmlaq_n_f32(vsubq_f32(_r07, _r01), vsubq_f32(_r03, _r05), 5.25f);
                        vst1q_f32(tmp[0][m], _tmp0m);
                        vst1q_f32(tmp[7][m], _tmp7m);

//                         tmp[0][m] = r0[0] - r0[6] + (r0[4] - r0[2]) * 5.25;
//                         tmp[7][m] = r0[7] - r0[1] + (r0[3] - r0[5]) * 5.25;

                        float32x4_t _tmp12a = vmlsq_n_f32(vaddq_f32(_r02, _r06), _r04, 4.25f);
                        float32x4_t _tmp12b = vmlsq_n_f32(vaddq_f32(_r01, _r05), _r03, 4.25f);

//                         float tmp12a = (r0[2] + r0[6] - r0[4] * 4.25);
//                         float tmp12b = (r0[1] + r0[5] - r0[3] * 4.25);

                        float32x4_t _tmp1m = vaddq_f32(_tmp12a, _tmp12b);
                        float32x4_t _tmp2m = vsubq_f32(_tmp12a, _tmp12b);
                        vst1q_f32(tmp[1][m], _tmp1m);
                        vst1q_f32(tmp[2][m], _tmp2m);

//                         tmp[1][m] = tmp12a + tmp12b;
//                         tmp[2][m] = tmp12a - tmp12b;

                        float32x4_t _tmp34a = vmlsq_n_f32(vmlaq_n_f32(_r06, _r02, 0.25f), _r04, 1.25f);
                        float32x4_t _tmp34b = vmlaq_n_f32(vmlsq_n_f32(vmulq_n_f32(_r01, 0.5f), _r03, 2.5f), _r05, 2.f);

//                         float tmp34a = (r0[6] + r0[2] * 0.25 - r0[4] * 1.25);
//                         float tmp34b = (r0[1] * 0.5 - r0[3] * 2.5 + r0[5] * 2);

                        float32x4_t _tmp3m = vaddq_f32(_tmp34a, _tmp34b);
                        float32x4_t _tmp4m = vsubq_f32(_tmp34a, _tmp34b);
                        vst1q_f32(tmp[3][m], _tmp3m);
                        vst1q_f32(tmp[4][m], _tmp4m);

//                         tmp[3][m] = tmp34a + tmp34b;
//                         tmp[4][m] = tmp34a - tmp34b;

                        float32x4_t _tmp56a = vmlaq_n_f32(_r06, vmlsq_n_f32(_r02, _r04, 1.25f), 4.f);
                        float32x4_t _tmp56b = vmlaq_n_f32(vmlsq_n_f32(vmulq_n_f32(_r01, 2.f), _r03, 2.5f), _r05, 0.5f);

//                         float tmp56a = (r0[6] + (r0[2] - r0[4] * 1.25) * 4);
//                         float tmp56b = (r0[1] * 2 - r0[3] * 2.5 + r0[5] * 0.5);

                        float32x4_t _tmp5m = vaddq_f32(_tmp56a, _tmp56b);
                        float32x4_t _tmp6m = vsubq_f32(_tmp56a, _tmp56b);
                        vst1q_f32(tmp[5][m], _tmp5m);
                        vst1q_f32(tmp[6][m], _tmp6m);

//                         tmp[5][m] = tmp56a + tmp56b;
//                         tmp[6][m] = tmp56a - tmp56b;

                        r0 += w * 4;
                    }

                    float* r0_tm_0 = (float*)img0_tm + (i * w_tm/8 + j) * 4;
                    float* r0_tm_1 = r0_tm_0 + tiles * 4;
                    float* r0_tm_2 = r0_tm_0 + tiles * 8;
                    float* r0_tm_3 = r0_tm_0 + tiles * 12;
                    float* r0_tm_4 = r0_tm_0 + tiles * 16;
                    float* r0_tm_5 = r0_tm_0 + tiles * 20;
                    float* r0_tm_6 = r0_tm_0 + tiles * 24;
                    float* r0_tm_7 = r0_tm_0 + tiles * 28;

                    for (int m=0; m<8; m++)
                    {
                        float32x4_t _tmp00 = vld1q_f32(tmp[m][0]);
                        float32x4_t _tmp01 = vld1q_f32(tmp[m][1]);
                        float32x4_t _tmp02 = vld1q_f32(tmp[m][2]);
                        float32x4_t _tmp03 = vld1q_f32(tmp[m][3]);
                        float32x4_t _tmp04 = vld1q_f32(tmp[m][4]);
                        float32x4_t _tmp05 = vld1q_f32(tmp[m][5]);
                        float32x4_t _tmp06 = vld1q_f32(tmp[m][6]);
                        float32x4_t _tmp07 = vld1q_f32(tmp[m][7]);

                        float32x4_t _r0tm0 = vmlaq_n_f32(vsubq_f32(_tmp00, _tmp06), vsubq_f32(_tmp04, _tmp02), 5.25f);
                        float32x4_t _r0tm7 = vmlaq_n_f32(vsubq_f32(_tmp07, _tmp01), vsubq_f32(_tmp03, _tmp05), 5.25f);

//                         r0_tm[0] = tmp0[0] - tmp0[6] + (tmp0[4] - tmp0[2]) * 5.25;
//                         r0_tm[7] = tmp0[7] - tmp0[1] + (tmp0[3] - tmp0[5]) * 5.25;

                        float32x4_t _tmp12a = vmlsq_n_f32(vaddq_f32(_tmp02, _tmp06), _tmp04, 4.25f);
                        float32x4_t _tmp12b = vmlsq_n_f32(vaddq_f32(_tmp01, _tmp05), _tmp03, 4.25f);

//                         float tmp12a = (tmp0[2] + tmp0[6] - tmp0[4] * 4.25);
//                         float tmp12b = (tmp0[1] + tmp0[5] - tmp0[3] * 4.25);

                        float32x4_t _r0tm1 = vaddq_f32(_tmp12a, _tmp12b);
                        float32x4_t _r0tm2 = vsubq_f32(_tmp12a, _tmp12b);

//                         r0_tm[1] = tmp12a + tmp12b;
//                         r0_tm[2] = tmp12a - tmp12b;

                        float32x4_t _tmp34a = vmlsq_n_f32(vmlaq_n_f32(_tmp06, _tmp02, 0.25f), _tmp04, 1.25f);
                        float32x4_t _tmp34b = vmlaq_n_f32(vmlsq_n_f32(vmulq_n_f32(_tmp01, 0.5f), _tmp03, 2.5f), _tmp05, 2.f);

//                         float tmp34a = (tmp0[6] + tmp0[2] * 0.25 - tmp0[4] * 1.25);
//                         float tmp34b = (tmp0[1] * 0.5 - tmp0[3] * 2.5 + tmp0[5] * 2);

                        float32x4_t _r0tm3 = vaddq_f32(_tmp34a, _tmp34b);
                        float32x4_t _r0tm4 = vsubq_f32(_tmp34a, _tmp34b);

//                         r0_tm[3] = tmp34a + tmp34b;
//                         r0_tm[4] = tmp34a - tmp34b;

                        float32x4_t _tmp56a = vmlaq_n_f32(_tmp06, vmlsq_n_f32(_tmp02, _tmp04, 1.25f), 4.f);
                        float32x4_t _tmp56b = vmlaq_n_f32(vmlsq_n_f32(vmulq_n_f32(_tmp01, 2.f), _tmp03, 2.5f), _tmp05, 0.5f);

//                         float tmp56a = (tmp0[6] + (tmp0[2] - tmp0[4] * 1.25) * 4);
//                         float tmp56b = (tmp0[1] * 2 - tmp0[3] * 2.5 + tmp0[5] * 0.5);

                        float32x4_t _r0tm5 = vaddq_f32(_tmp56a, _tmp56b);
                        float32x4_t _r0tm6 = vsubq_f32(_tmp56a, _tmp56b);

//                         r0_tm[5] = tmp56a + tmp56b;
//                         r0_tm[6] = tmp56a - tmp56b;

                        vst1q_f32(r0_tm_0, _r0tm0);
                        vst1q_f32(r0_tm_1, _r0tm1);
                        vst1q_f32(r0_tm_2, _r0tm2);
                        vst1q_f32(r0_tm_3, _r0tm3);
                        vst1q_f32(r0_tm_4, _r0tm4);
                        vst1q_f32(r0_tm_5, _r0tm5);
                        vst1q_f32(r0_tm_6, _r0tm6);
                        vst1q_f32(r0_tm_7, _r0tm7);

                        r0_tm_0 += tiles * 32;
                        r0_tm_1 += tiles * 32;
                        r0_tm_2 += tiles * 32;
                        r0_tm_3 += tiles * 32;
                        r0_tm_4 += tiles * 32;
                        r0_tm_5 += tiles * 32;
                        r0_tm_6 += tiles * 32;
                        r0_tm_7 += tiles * 32;
                    }
                }
            }
        }

    }
    bottom_blob_bordered = Mat();
    // END transform input

    // BEGIN dot
    Mat top_blob_tm;
    {
        int w_tm = outw / 6 * 8;
        int h_tm = outh / 6 * 8;

        const int tiles = h_tm/8 * w_tm/8;

        // permute
//         bottom_blob_tm.create(tiles, 64, inch, elemsize, elempack, opt.workspace_allocator);
#if __aarch64__
        Mat bottom_blob_tm2(12 * inch, tiles/12 + (tiles%12)/8 + (tiles%8)/4 + (tiles%4)/2 + tiles%2, 64, elemsize, elempack, opt.workspace_allocator);
#else
        Mat bottom_blob_tm2(8 * inch, tiles/8 + (tiles%8)/4 + (tiles%4)/2 + tiles%2, 64, elemsize, elempack, opt.workspace_allocator);
#endif

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int r=0; r<64; r++)
        {
            Mat tm2 = bottom_blob_tm2.channel(r);

            // tile
            int i=0;
#if __aarch64__
            for (; i+11<tiles; i+=12)
            {
                float* tm2p = tm2.row(i/12);

                const float* r0 = bottom_blob_tm;

                r0 += (r*tiles + i) * 4;

                for (int q=0; q<inch; q++)
                {
                    asm volatile(
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld4    {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64 \n"
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld4    {v4.4s, v5.4s, v6.4s, v7.4s}, [%0], #64 \n"
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld4    {v8.4s, v9.4s, v10.4s, v11.4s}, [%0] \n"
                        "st1    {v0.4s}, [%1], #16          \n"
                        "st1    {v4.4s}, [%1], #16          \n"
                        "st1    {v8.4s}, [%1], #16          \n"
                        "sub    %0, %0, #128                \n"
                        "st1    {v1.4s}, [%1], #16          \n"
                        "st1    {v5.4s}, [%1], #16          \n"
                        "st1    {v9.4s}, [%1], #16          \n"
                        "st1    {v2.4s}, [%1], #16          \n"
                        "st1    {v6.4s}, [%1], #16          \n"
                        "st1    {v10.4s}, [%1], #16         \n"
                        "st1    {v3.4s}, [%1], #16          \n"
                        "st1    {v7.4s}, [%1], #16          \n"
                        "st1    {v11.4s}, [%1], #16         \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11"
                    );
                    r0 += bottom_blob_tm.cstep * 4;
                }
            }
#endif
            for (; i+7<tiles; i+=8)
            {
#if __aarch64__
                float* tm2p = tm2.row(i/12 + (i%12)/8);
#else
                float* tm2p = tm2.row(i/8);
#endif

                const float* r0 = bottom_blob_tm;

                r0 += (r*tiles + i) * 4;

                for (int q=0; q<inch; q++)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%0], #64 \n"
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%0] \n"
                        "st1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%1], #64 \n"
                        "sub    %0, %0, #64                 \n"
                        "st1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%1], #64 \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7"
                    );
#else
                    asm volatile(
                        "pld        [%0, #512]          \n"
                        "vldm       %0!, {d0-d7}        \n"
                        "pld        [%0, #512]          \n"
                        "vldm       %0, {d16-d23}       \n"

                        // transpose 8x4
                        "vtrn.32    q0, q1              \n"
                        "vtrn.32    q2, q3              \n"
                        "vtrn.32    q8, q9              \n"
                        "vtrn.32    q10, q11            \n"
                        "vswp       d1, d4              \n"
                        "vswp       d3, d6              \n"
                        "vswp       d17, d20            \n"
                        "vswp       d19, d22            \n"
                        "vswp       q1, q8              \n"
                        "vswp       q3, q10             \n"

                        "vst1.f32   {d0-d3}, [%1 :128]! \n"
                        "vst1.f32   {d16-d19}, [%1 :128]! \n"
                        "sub        %0, %0, #64         \n"
                        "vst1.f32   {d4-d7}, [%1 :128]! \n"
                        "vst1.f32   {d20-d23}, [%1 :128]! \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11"
                    );
#endif
                    r0 += bottom_blob_tm.cstep * 4;
                }
            }
            for (; i+3<tiles; i+=4)
            {
#if __aarch64__
                float* tm2p = tm2.row(i/12 + (i%12)/8 + (i%8)/4);
#else
                float* tm2p = tm2.row(i/8 + (i%8)/4);
#endif

                const float* r0 = bottom_blob_tm;

                r0 += (r*tiles + i) * 4;

                for (int q=0; q<inch; q++)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%0] \n"
                        "st1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%1], #64 \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "v0", "v1", "v2", "v3"
                    );
#else
                    asm volatile(
                        "pld        [%0, #512]          \n"
                        "vldm       %0, {d0-d7}         \n"
                        "vstm       %1!, {d0-d7}        \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "q0", "q1", "q2", "q3"
                    );
#endif // __aarch64__
                    r0 += bottom_blob_tm.cstep * 4;
                }
            }
            for (; i+1<tiles; i+=2)
            {
#if __aarch64__
                float* tm2p = tm2.row(i/12 + (i%12)/8 + (i%8)/4 + (i%4)/2);
#else
                float* tm2p = tm2.row(i/8 + (i%8)/4 + (i%4)/2);
#endif

                const float* r0 = bottom_blob_tm;

                r0 += (r*tiles + i) * 4;

                for (int q=0; q<inch; q++)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #256]       \n"
                        "ld1    {v0.4s, v1.4s}, [%0]        \n"
                        "st1    {v0.4s, v1.4s}, [%1], #32   \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "v0", "v1"
                    );
#else
                    asm volatile(
                        "pld        [%0, #256]          \n"
                        "vld1.f32   {d0-d3}, [%0 :128]  \n"
                        "vst1.f32   {d0-d3}, [%1 :128]! \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "q0", "q1"
                    );
#endif // __aarch64__
                    r0 += bottom_blob_tm.cstep * 4;
                }
            }
            for (; i<tiles; i++)
            {
#if __aarch64__
                float* tm2p = tm2.row(i/12 + (i%12)/8 + (i%8)/4 + (i%4)/2 + i%2);
#else
                float* tm2p = tm2.row(i/8 + (i%8)/4 + (i%4)/2 + i%2);
#endif

                const float* r0 = bottom_blob_tm;

                r0 += (r*tiles + i) * 4;

                for (int q=0; q<inch; q++)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #128]       \n"
                        "ld1    {v0.4s}, [%0]               \n"
                        "st1    {v0.4s}, [%1], #16          \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "v0"
                    );
#else
                    asm volatile(
                        "pld        [%0, #128]          \n"
                        "vld1.f32   {d0-d1}, [%0 :128]  \n"
                        "vst1.f32   {d0-d1}, [%1 :128]! \n"
                        : "=r"(r0),     // %0
                          "=r"(tm2p)    // %1
                        : "0"(r0),
                          "1"(tm2p)
                        : "memory", "q0"
                    );
#endif // __aarch64__
                    r0 += bottom_blob_tm.cstep * 4;
                }
            }
        }

        bottom_blob_tm = Mat();
        // permute end

        top_blob_tm.create(tiles, 64, outch, elemsize, elempack);

        int nn_outch = 0;
        int remain_outch_start = 0;

#if __ARM_NEON && __aarch64__
        nn_outch = outch >> 1;
        remain_outch_start = nn_outch << 1;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int pp=0; pp<nn_outch; pp++)
        {
            int p = pp * 2;

            float* output0_tm = top_blob_tm.channel(p);
            float* output1_tm = top_blob_tm.channel(p+1);

            const Mat kernel01_tm = kernel_tm.channel(pp);

            for (int r=0; r<64; r++)
            {
                const Mat bb2 = bottom_blob_tm2.channel(r);

                int i=0;
                for (; i+11<tiles; i+=12)
                {
                    const float* r0 = bb2.row(i/12);

                    const float* k01 = kernel01_tm.row(r);

                    int nn = inch;// inch always > 0

                    asm volatile(
                        "eor    v8.16b, v8.16b, v8.16b      \n"
                        "eor    v9.16b, v9.16b, v9.16b      \n"
                        "eor    v10.16b, v10.16b, v10.16b   \n"
                        "eor    v11.16b, v11.16b, v11.16b   \n"
                        "eor    v12.16b, v12.16b, v12.16b   \n"
                        "eor    v13.16b, v13.16b, v13.16b   \n"
                        "eor    v14.16b, v14.16b, v14.16b   \n"
                        "eor    v15.16b, v15.16b, v15.16b   \n"
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"
                        "eor    v20.16b, v20.16b, v20.16b   \n"
                        "eor    v21.16b, v21.16b, v21.16b   \n"
                        "eor    v22.16b, v22.16b, v22.16b   \n"
                        "eor    v23.16b, v23.16b, v23.16b   \n"
                        "eor    v24.16b, v24.16b, v24.16b   \n"
                        "eor    v25.16b, v25.16b, v25.16b   \n"
                        "eor    v26.16b, v26.16b, v26.16b   \n"
                        "eor    v27.16b, v27.16b, v27.16b   \n"
                        "eor    v28.16b, v28.16b, v28.16b   \n"
                        "eor    v29.16b, v29.16b, v29.16b   \n"
                        "eor    v30.16b, v30.16b, v30.16b   \n"
                        "eor    v31.16b, v31.16b, v31.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%4], #64   \n"// w0011_01

                        "fmla   v8.4s, v4.4s, v0.s[0]       \n"
                        "fmla   v9.4s, v4.4s, v0.s[1]       \n"
                        "fmla   v10.4s, v4.4s, v0.s[2]      \n"
                        "fmla   v11.4s, v4.4s, v0.s[3]      \n"
                        "fmla   v12.4s, v4.4s, v1.s[0]      \n"
                        "fmla   v13.4s, v4.4s, v1.s[1]      \n"
                        "fmla   v14.4s, v4.4s, v1.s[2]      \n"
                        "fmla   v15.4s, v4.4s, v1.s[3]      \n"
                        "fmla   v16.4s, v4.4s, v2.s[0]      \n"
                        "fmla   v17.4s, v4.4s, v2.s[1]      \n"
                        "fmla   v18.4s, v4.4s, v2.s[2]      \n"
                        "fmla   v19.4s, v4.4s, v2.s[3]      \n"

                        "fmla   v20.4s, v5.4s, v0.s[0]      \n"
                        "fmla   v21.4s, v5.4s, v0.s[1]      \n"
                        "fmla   v22.4s, v5.4s, v0.s[2]      \n"
                        "fmla   v23.4s, v5.4s, v0.s[3]      \n"
                        "fmla   v24.4s, v5.4s, v1.s[0]      \n"
                        "fmla   v25.4s, v5.4s, v1.s[1]      \n"
                        "fmla   v26.4s, v5.4s, v1.s[2]      \n"
                        "fmla   v27.4s, v5.4s, v1.s[3]      \n"
                        "fmla   v28.4s, v5.4s, v2.s[0]      \n"
                        "fmla   v29.4s, v5.4s, v2.s[1]      \n"
                        "fmla   v30.4s, v5.4s, v2.s[2]      \n"
                        "fmla   v31.4s, v5.4s, v2.s[3]      \n"

                        "fmla   v8.4s, v6.4s, v3.s[0]       \n"
                        "fmla   v9.4s, v6.4s, v3.s[1]       \n"
                        "fmla   v10.4s, v6.4s, v3.s[2]      \n"
                        "fmla   v11.4s, v6.4s, v3.s[3]      \n"

                        "fmla   v20.4s, v7.4s, v3.s[0]      \n"
                        "fmla   v21.4s, v7.4s, v3.s[1]      \n"
                        "fmla   v22.4s, v7.4s, v3.s[2]      \n"
                        "fmla   v23.4s, v7.4s, v3.s[3]      \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"

                        "fmla   v12.4s, v6.4s, v0.s[0]      \n"
                        "fmla   v13.4s, v6.4s, v0.s[1]      \n"
                        "fmla   v14.4s, v6.4s, v0.s[2]      \n"
                        "fmla   v15.4s, v6.4s, v0.s[3]      \n"
                        "fmla   v16.4s, v6.4s, v1.s[0]      \n"
                        "fmla   v17.4s, v6.4s, v1.s[1]      \n"
                        "fmla   v18.4s, v6.4s, v1.s[2]      \n"
                        "fmla   v19.4s, v6.4s, v1.s[3]      \n"

                        "fmla   v24.4s, v7.4s, v0.s[0]      \n"
                        "fmla   v25.4s, v7.4s, v0.s[1]      \n"
                        "fmla   v26.4s, v7.4s, v0.s[2]      \n"
                        "fmla   v27.4s, v7.4s, v0.s[3]      \n"
                        "fmla   v28.4s, v7.4s, v1.s[0]      \n"
                        "fmla   v29.4s, v7.4s, v1.s[1]      \n"
                        "fmla   v30.4s, v7.4s, v1.s[2]      \n"
                        "fmla   v31.4s, v7.4s, v1.s[3]      \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%4], #64   \n"// w2233_01

                        "fmla   v8.4s, v4.4s, v2.s[0]       \n"
                        "fmla   v9.4s, v4.4s, v2.s[1]       \n"
                        "fmla   v10.4s, v4.4s, v2.s[2]      \n"
                        "fmla   v11.4s, v4.4s, v2.s[3]      \n"
                        "fmla   v12.4s, v4.4s, v3.s[0]      \n"
                        "fmla   v13.4s, v4.4s, v3.s[1]      \n"
                        "fmla   v14.4s, v4.4s, v3.s[2]      \n"
                        "fmla   v15.4s, v4.4s, v3.s[3]      \n"

                        "fmla   v20.4s, v5.4s, v2.s[0]      \n"
                        "fmla   v21.4s, v5.4s, v2.s[1]      \n"
                        "fmla   v22.4s, v5.4s, v2.s[2]      \n"
                        "fmla   v23.4s, v5.4s, v2.s[3]      \n"
                        "fmla   v24.4s, v5.4s, v3.s[0]      \n"
                        "fmla   v25.4s, v5.4s, v3.s[1]      \n"
                        "fmla   v26.4s, v5.4s, v3.s[2]      \n"
                        "fmla   v27.4s, v5.4s, v3.s[3]      \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"

                        "fmla   v16.4s, v4.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v4.4s, v0.s[1]      \n"
                        "fmla   v18.4s, v4.4s, v0.s[2]      \n"
                        "fmla   v19.4s, v4.4s, v0.s[3]      \n"

                        "fmla   v28.4s, v5.4s, v0.s[0]      \n"
                        "fmla   v29.4s, v5.4s, v0.s[1]      \n"
                        "fmla   v30.4s, v5.4s, v0.s[2]      \n"
                        "fmla   v31.4s, v5.4s, v0.s[3]      \n"

                        "fmla   v8.4s, v6.4s, v1.s[0]       \n"
                        "fmla   v9.4s, v6.4s, v1.s[1]       \n"
                        "fmla   v10.4s, v6.4s, v1.s[2]      \n"
                        "fmla   v11.4s, v6.4s, v1.s[3]      \n"
                        "fmla   v12.4s, v6.4s, v2.s[0]      \n"
                        "fmla   v13.4s, v6.4s, v2.s[1]      \n"
                        "fmla   v14.4s, v6.4s, v2.s[2]      \n"
                        "fmla   v15.4s, v6.4s, v2.s[3]      \n"
                        "fmla   v16.4s, v6.4s, v3.s[0]      \n"
                        "fmla   v17.4s, v6.4s, v3.s[1]      \n"
                        "fmla   v18.4s, v6.4s, v3.s[2]      \n"
                        "fmla   v19.4s, v6.4s, v3.s[3]      \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v20.4s, v7.4s, v1.s[0]      \n"
                        "fmla   v21.4s, v7.4s, v1.s[1]      \n"
                        "fmla   v22.4s, v7.4s, v1.s[2]      \n"
                        "fmla   v23.4s, v7.4s, v1.s[3]      \n"
                        "fmla   v24.4s, v7.4s, v2.s[0]      \n"
                        "fmla   v25.4s, v7.4s, v2.s[1]      \n"
                        "fmla   v26.4s, v7.4s, v2.s[2]      \n"
                        "fmla   v27.4s, v7.4s, v2.s[3]      \n"
                        "fmla   v28.4s, v7.4s, v3.s[0]      \n"
                        "fmla   v29.4s, v7.4s, v3.s[1]      \n"
                        "fmla   v30.4s, v7.4s, v3.s[2]      \n"
                        "fmla   v31.4s, v7.4s, v3.s[3]      \n"

                        "bne    0b                          \n"

                        "st1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%1], #64 \n"
                        "st1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%2], #64 \n"
                        "st1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%1], #64 \n"
                        "st1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%2], #64 \n"
                        "st1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 \n"
                        "st1    {v28.4s, v29.4s, v30.4s, v31.4s}, [%2], #64 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(output1_tm), // %2
                          "=r"(r0),         // %3
                          "=r"(k01)         // %4
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(output1_tm),
                          "3"(r0),
                          "4"(k01)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"
                    );
                }
                for (; i+7<tiles; i+=8)
                {
                    const float* r0 = bb2.row(i/12 + (i%12)/8);

                    const float* k01 = kernel01_tm.row(r);

                    int nn = inch;// inch always > 0

                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"
                        "eor    v20.16b, v20.16b, v20.16b   \n"
                        "eor    v21.16b, v21.16b, v21.16b   \n"
                        "eor    v22.16b, v22.16b, v22.16b   \n"
                        "eor    v23.16b, v23.16b, v23.16b   \n"
                        "eor    v24.16b, v24.16b, v24.16b   \n"
                        "eor    v25.16b, v25.16b, v25.16b   \n"
                        "eor    v26.16b, v26.16b, v26.16b   \n"
                        "eor    v27.16b, v27.16b, v27.16b   \n"
                        "eor    v28.16b, v28.16b, v28.16b   \n"
                        "eor    v29.16b, v29.16b, v29.16b   \n"
                        "eor    v30.16b, v30.16b, v30.16b   \n"
                        "eor    v31.16b, v31.16b, v31.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"// r0 r1 r2 r3

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%4], #64 \n"// w0011_01

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%3], #64 \n"// r4 r5 r6 r7

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v8.4s, v1.s[0]      \n"
                        "fmla   v18.4s, v8.4s, v2.s[0]      \n"
                        "fmla   v19.4s, v8.4s, v3.s[0]      \n"
                        "fmla   v20.4s, v8.4s, v4.s[0]      \n"
                        "fmla   v21.4s, v8.4s, v5.s[0]      \n"
                        "fmla   v22.4s, v8.4s, v6.s[0]      \n"
                        "fmla   v23.4s, v8.4s, v7.s[0]      \n"

                        "fmla   v24.4s, v9.4s, v0.s[0]      \n"
                        "fmla   v25.4s, v9.4s, v1.s[0]      \n"
                        "fmla   v26.4s, v9.4s, v2.s[0]      \n"
                        "fmla   v27.4s, v9.4s, v3.s[0]      \n"
                        "fmla   v28.4s, v9.4s, v4.s[0]      \n"
                        "fmla   v29.4s, v9.4s, v5.s[0]      \n"
                        "fmla   v30.4s, v9.4s, v6.s[0]      \n"
                        "fmla   v31.4s, v9.4s, v7.s[0]      \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%4], #64 \n"// w2233_01

                        "fmla   v16.4s, v10.4s, v0.s[1]     \n"
                        "fmla   v17.4s, v10.4s, v1.s[1]     \n"
                        "fmla   v18.4s, v10.4s, v2.s[1]     \n"
                        "fmla   v19.4s, v10.4s, v3.s[1]     \n"
                        "fmla   v20.4s, v10.4s, v4.s[1]     \n"
                        "fmla   v21.4s, v10.4s, v5.s[1]     \n"
                        "fmla   v22.4s, v10.4s, v6.s[1]     \n"
                        "fmla   v23.4s, v10.4s, v7.s[1]     \n"

                        "fmla   v24.4s, v11.4s, v0.s[1]     \n"
                        "fmla   v25.4s, v11.4s, v1.s[1]     \n"
                        "fmla   v26.4s, v11.4s, v2.s[1]     \n"
                        "fmla   v27.4s, v11.4s, v3.s[1]     \n"
                        "fmla   v28.4s, v11.4s, v4.s[1]     \n"
                        "fmla   v29.4s, v11.4s, v5.s[1]     \n"
                        "fmla   v30.4s, v11.4s, v6.s[1]     \n"
                        "fmla   v31.4s, v11.4s, v7.s[1]     \n"

                        "fmla   v16.4s, v12.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v12.4s, v1.s[2]     \n"
                        "fmla   v18.4s, v12.4s, v2.s[2]     \n"
                        "fmla   v19.4s, v12.4s, v3.s[2]     \n"
                        "fmla   v20.4s, v12.4s, v4.s[2]     \n"
                        "fmla   v21.4s, v12.4s, v5.s[2]     \n"
                        "fmla   v22.4s, v12.4s, v6.s[2]     \n"
                        "fmla   v23.4s, v12.4s, v7.s[2]     \n"

                        "fmla   v24.4s, v13.4s, v0.s[2]     \n"
                        "fmla   v25.4s, v13.4s, v1.s[2]     \n"
                        "fmla   v26.4s, v13.4s, v2.s[2]     \n"
                        "fmla   v27.4s, v13.4s, v3.s[2]     \n"
                        "fmla   v28.4s, v13.4s, v4.s[2]     \n"
                        "fmla   v29.4s, v13.4s, v5.s[2]     \n"
                        "fmla   v30.4s, v13.4s, v6.s[2]     \n"
                        "fmla   v31.4s, v13.4s, v7.s[2]     \n"

                        "fmla   v16.4s, v14.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v14.4s, v1.s[3]     \n"
                        "fmla   v18.4s, v14.4s, v2.s[3]     \n"
                        "fmla   v19.4s, v14.4s, v3.s[3]     \n"
                        "fmla   v20.4s, v14.4s, v4.s[3]     \n"
                        "fmla   v21.4s, v14.4s, v5.s[3]     \n"
                        "fmla   v22.4s, v14.4s, v6.s[3]     \n"
                        "fmla   v23.4s, v14.4s, v7.s[3]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v24.4s, v15.4s, v0.s[3]     \n"
                        "fmla   v25.4s, v15.4s, v1.s[3]     \n"
                        "fmla   v26.4s, v15.4s, v2.s[3]     \n"
                        "fmla   v27.4s, v15.4s, v3.s[3]     \n"
                        "fmla   v28.4s, v15.4s, v4.s[3]     \n"
                        "fmla   v29.4s, v15.4s, v5.s[3]     \n"
                        "fmla   v30.4s, v15.4s, v6.s[3]     \n"
                        "fmla   v31.4s, v15.4s, v7.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 \n"
                        "st1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%2], #64 \n"
                        "st1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%1], #64 \n"
                        "st1    {v28.4s, v29.4s, v30.4s, v31.4s}, [%2], #64 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(output1_tm), // %2
                          "=r"(r0),         // %3
                          "=r"(k01)         // %4
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(output1_tm),
                          "3"(r0),
                          "4"(k01)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"
                    );
                }
                for (; i+3<tiles; i+=4)
                {
                    const float* r0 = bb2.row(i/12 + (i%12)/8 + (i%8)/4);

                    const float* k01 = kernel01_tm.row(r);

                    int nn = inch;// inch always > 0

                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"
                        "eor    v20.16b, v20.16b, v20.16b   \n"
                        "eor    v21.16b, v21.16b, v21.16b   \n"
                        "eor    v22.16b, v22.16b, v22.16b   \n"
                        "eor    v23.16b, v23.16b, v23.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"// r0 r1 r2 r3

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%4], #64 \n"// w0011_01

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v8.4s, v1.s[0]      \n"
                        "fmla   v18.4s, v8.4s, v2.s[0]      \n"
                        "fmla   v19.4s, v8.4s, v3.s[0]      \n"

                        "fmla   v20.4s, v9.4s, v0.s[0]     \n"
                        "fmla   v21.4s, v9.4s, v1.s[0]     \n"
                        "fmla   v22.4s, v9.4s, v2.s[0]     \n"
                        "fmla   v23.4s, v9.4s, v3.s[0]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%4], #64 \n"// w2233_01

                        "fmla   v16.4s, v10.4s, v0.s[1]      \n"
                        "fmla   v17.4s, v10.4s, v1.s[1]      \n"
                        "fmla   v18.4s, v10.4s, v2.s[1]      \n"
                        "fmla   v19.4s, v10.4s, v3.s[1]      \n"

                        "fmla   v20.4s, v11.4s, v0.s[1]     \n"
                        "fmla   v21.4s, v11.4s, v1.s[1]     \n"
                        "fmla   v22.4s, v11.4s, v2.s[1]     \n"
                        "fmla   v23.4s, v11.4s, v3.s[1]     \n"

                        "fmla   v16.4s, v12.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v12.4s, v1.s[2]     \n"
                        "fmla   v18.4s, v12.4s, v2.s[2]     \n"
                        "fmla   v19.4s, v12.4s, v3.s[2]     \n"

                        "fmla   v20.4s, v13.4s, v0.s[2]     \n"
                        "fmla   v21.4s, v13.4s, v1.s[2]     \n"
                        "fmla   v22.4s, v13.4s, v2.s[2]     \n"
                        "fmla   v23.4s, v13.4s, v3.s[2]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v14.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v14.4s, v1.s[3]     \n"
                        "fmla   v18.4s, v14.4s, v2.s[3]     \n"
                        "fmla   v19.4s, v14.4s, v3.s[3]     \n"

                        "fmla   v20.4s, v15.4s, v0.s[3]     \n"
                        "fmla   v21.4s, v15.4s, v1.s[3]     \n"
                        "fmla   v22.4s, v15.4s, v2.s[3]     \n"
                        "fmla   v23.4s, v15.4s, v3.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 \n"
                        "st1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%2], #64 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(output1_tm), // %2
                          "=r"(r0),         // %3
                          "=r"(k01)         // %4
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(output1_tm),
                          "3"(r0),
                          "4"(k01)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23"
                    );
                }
                for (; i+1<tiles; i+=2)
                {
                    const float* r0 = bb2.row(i/12 + (i%12)/8 + (i%8)/4 + (i%4)/2);

                    const float* k01 = kernel01_tm.row(r);

                    int nn = inch;// inch always > 0

                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%3, #256]       \n"
                        "ld1    {v0.4s, v1.4s}, [%3], #32   \n"// r0 r1

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%4], #64 \n"// w0011_01

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v8.4s, v1.s[0]      \n"
                        "fmla   v18.4s, v9.4s, v0.s[0]      \n"
                        "fmla   v19.4s, v9.4s, v1.s[0]      \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%4], #64 \n"// w2233_01

                        "fmla   v16.4s, v10.4s, v0.s[1]     \n"
                        "fmla   v17.4s, v10.4s, v1.s[1]     \n"
                        "fmla   v18.4s, v11.4s, v0.s[1]     \n"
                        "fmla   v19.4s, v11.4s, v1.s[1]     \n"

                        "fmla   v16.4s, v12.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v12.4s, v1.s[2]     \n"
                        "fmla   v18.4s, v13.4s, v0.s[2]     \n"
                        "fmla   v19.4s, v13.4s, v1.s[2]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v14.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v14.4s, v1.s[3]     \n"
                        "fmla   v18.4s, v15.4s, v0.s[3]     \n"
                        "fmla   v19.4s, v15.4s, v1.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s, v17.4s}, [%1], #32 \n"
                        "st1    {v18.4s, v19.4s}, [%2], #32 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(output1_tm), // %2
                          "=r"(r0),         // %3
                          "=r"(k01)         // %4
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(output1_tm),
                          "3"(r0),
                          "4"(k01)
                        : "cc", "memory", "v0", "v1", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19"
                    );
                }
                for (; i<tiles; i++)
                {
                    const float* r0 = bb2.row(i/12 + (i%12)/8 + (i%8)/4 + (i%4)/2 + i%2);

                    const float* k01 = kernel01_tm.row(r);

                    int nn = inch;// inch always > 0

                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%3, #128]       \n"
                        "ld1    {v0.4s}, [%3], #16          \n"// r0

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%4], #64 \n"// w0011_01

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v9.4s, v0.s[0]      \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%4], #64 \n"// w2233_01

                        "fmla   v16.4s, v10.4s, v0.s[1]     \n"
                        "fmla   v17.4s, v11.4s, v0.s[1]     \n"

                        "fmla   v16.4s, v12.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v13.4s, v0.s[2]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v14.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v15.4s, v0.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s}, [%1], #16         \n"
                        "st1    {v17.4s}, [%2], #16         \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(output1_tm), // %2
                          "=r"(r0),         // %3
                          "=r"(k01)         // %4
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(output1_tm),
                          "3"(r0),
                          "4"(k01)
                        : "cc", "memory", "v0", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17"
                    );
                }

            }
        }
#endif // __ARM_NEON && __aarch64__

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p=remain_outch_start; p<outch; p++)
        {
            float* output0_tm = top_blob_tm.channel(p);

#if __aarch64__
            const Mat kernel0_tm = kernel_tm.channel(p/2+p%2);
#else
            const Mat kernel0_tm = kernel_tm.channel(p);
#endif

            for (int r=0; r<64; r++)
            {
                const Mat bb2 = bottom_blob_tm2.channel(r);

                int i=0;
#if __aarch64__
                for (; i+11<tiles; i+=12)
                {
                    const float* r0 = bb2.row(i/12);

                    const float* k0 = kernel0_tm.row(r);

                    int nn = inch;// inch always > 0

                    asm volatile(
                        "eor    v8.16b, v8.16b, v8.16b      \n"
                        "eor    v9.16b, v9.16b, v9.16b      \n"
                        "eor    v10.16b, v10.16b, v10.16b   \n"
                        "eor    v11.16b, v11.16b, v11.16b   \n"
                        "eor    v12.16b, v12.16b, v12.16b   \n"
                        "eor    v13.16b, v13.16b, v13.16b   \n"
                        "eor    v14.16b, v14.16b, v14.16b   \n"
                        "eor    v15.16b, v15.16b, v15.16b   \n"
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%2], #64 \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%3], #64 \n"// w0123_0

                        "fmla   v8.4s, v4.4s, v0.s[0]       \n"
                        "fmla   v9.4s, v4.4s, v0.s[1]       \n"
                        "fmla   v10.4s, v4.4s, v0.s[2]      \n"
                        "fmla   v11.4s, v4.4s, v0.s[3]      \n"
                        "fmla   v12.4s, v4.4s, v1.s[0]      \n"
                        "fmla   v13.4s, v4.4s, v1.s[1]      \n"
                        "fmla   v14.4s, v4.4s, v1.s[2]      \n"
                        "fmla   v15.4s, v4.4s, v1.s[3]      \n"
                        "fmla   v16.4s, v4.4s, v2.s[0]      \n"
                        "fmla   v17.4s, v4.4s, v2.s[1]      \n"
                        "fmla   v18.4s, v4.4s, v2.s[2]      \n"
                        "fmla   v19.4s, v4.4s, v2.s[3]      \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%2], #64 \n"

                        "fmla   v8.4s, v5.4s, v3.s[0]       \n"
                        "fmla   v9.4s, v5.4s, v3.s[1]       \n"
                        "fmla   v10.4s, v5.4s, v3.s[2]      \n"
                        "fmla   v11.4s, v5.4s, v3.s[3]      \n"
                        "fmla   v12.4s, v5.4s, v20.s[0]     \n"
                        "fmla   v13.4s, v5.4s, v20.s[1]     \n"
                        "fmla   v14.4s, v5.4s, v20.s[2]     \n"
                        "fmla   v15.4s, v5.4s, v20.s[3]     \n"
                        "fmla   v16.4s, v5.4s, v21.s[0]     \n"
                        "fmla   v17.4s, v5.4s, v21.s[1]     \n"
                        "fmla   v18.4s, v5.4s, v21.s[2]     \n"
                        "fmla   v19.4s, v5.4s, v21.s[3]     \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%2], #64 \n"

                        "fmla   v8.4s, v6.4s, v22.s[0]      \n"
                        "fmla   v9.4s, v6.4s, v22.s[1]      \n"
                        "fmla   v10.4s, v6.4s, v22.s[2]     \n"
                        "fmla   v11.4s, v6.4s, v22.s[3]     \n"
                        "fmla   v12.4s, v6.4s, v23.s[0]     \n"
                        "fmla   v13.4s, v6.4s, v23.s[1]     \n"
                        "fmla   v14.4s, v6.4s, v23.s[2]     \n"
                        "fmla   v15.4s, v6.4s, v23.s[3]     \n"
                        "fmla   v16.4s, v6.4s, v24.s[0]     \n"
                        "fmla   v17.4s, v6.4s, v24.s[1]     \n"
                        "fmla   v18.4s, v6.4s, v24.s[2]     \n"
                        "fmla   v19.4s, v6.4s, v24.s[3]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v8.4s, v7.4s, v25.s[0]      \n"
                        "fmla   v9.4s, v7.4s, v25.s[1]      \n"
                        "fmla   v10.4s, v7.4s, v25.s[2]     \n"
                        "fmla   v11.4s, v7.4s, v25.s[3]     \n"
                        "fmla   v12.4s, v7.4s, v26.s[0]     \n"
                        "fmla   v13.4s, v7.4s, v26.s[1]     \n"
                        "fmla   v14.4s, v7.4s, v26.s[2]     \n"
                        "fmla   v15.4s, v7.4s, v26.s[3]     \n"
                        "fmla   v16.4s, v7.4s, v27.s[0]     \n"
                        "fmla   v17.4s, v7.4s, v27.s[1]     \n"
                        "fmla   v18.4s, v7.4s, v27.s[2]     \n"
                        "fmla   v19.4s, v7.4s, v27.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%1], #64 \n"
                        "st1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%1], #64 \n"
                        "st1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27"
                    );
                }
#endif
                for (; i+7<tiles; i+=8)
                {
#if __aarch64__
                    const float* r0 = bb2.row(i/12 + (i%12)/8);
#else
                    const float* r0 = bb2.row(i/8);
#endif

                    const float* k0 = kernel0_tm.row(r);

                    int nn = inch;// inch always > 0

#if __aarch64__
                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"
                        "eor    v20.16b, v20.16b, v20.16b   \n"
                        "eor    v21.16b, v21.16b, v21.16b   \n"
                        "eor    v22.16b, v22.16b, v22.16b   \n"
                        "eor    v23.16b, v23.16b, v23.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%2], #64 \n"// r0 r1 r2 r3

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%3], #64 \n"// w0123

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v8.4s, v1.s[0]      \n"
                        "fmla   v18.4s, v8.4s, v2.s[0]      \n"
                        "fmla   v19.4s, v8.4s, v3.s[0]      \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%2], #64 \n"// r4 r5 r6 r7

                        "fmla   v20.4s, v8.4s, v4.s[0]      \n"
                        "fmla   v21.4s, v8.4s, v5.s[0]      \n"
                        "fmla   v22.4s, v8.4s, v6.s[0]      \n"
                        "fmla   v23.4s, v8.4s, v7.s[0]      \n"

                        "fmla   v16.4s, v9.4s, v0.s[1]      \n"
                        "fmla   v17.4s, v9.4s, v1.s[1]      \n"
                        "fmla   v18.4s, v9.4s, v2.s[1]      \n"
                        "fmla   v19.4s, v9.4s, v3.s[1]      \n"
                        "fmla   v20.4s, v9.4s, v4.s[1]      \n"
                        "fmla   v21.4s, v9.4s, v5.s[1]      \n"
                        "fmla   v22.4s, v9.4s, v6.s[1]      \n"
                        "fmla   v23.4s, v9.4s, v7.s[1]      \n"

                        "fmla   v16.4s, v10.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v10.4s, v1.s[2]     \n"
                        "fmla   v18.4s, v10.4s, v2.s[2]     \n"
                        "fmla   v19.4s, v10.4s, v3.s[2]     \n"
                        "fmla   v20.4s, v10.4s, v4.s[2]     \n"
                        "fmla   v21.4s, v10.4s, v5.s[2]     \n"
                        "fmla   v22.4s, v10.4s, v6.s[2]     \n"
                        "fmla   v23.4s, v10.4s, v7.s[2]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v11.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v11.4s, v1.s[3]     \n"
                        "fmla   v18.4s, v11.4s, v2.s[3]     \n"
                        "fmla   v19.4s, v11.4s, v3.s[3]     \n"
                        "fmla   v20.4s, v11.4s, v4.s[3]     \n"
                        "fmla   v21.4s, v11.4s, v5.s[3]     \n"
                        "fmla   v22.4s, v11.4s, v6.s[3]     \n"
                        "fmla   v23.4s, v11.4s, v7.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 \n"
                        "st1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%1], #64 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23"
                    );
#else
                    asm volatile(
                        "veor       q8, q8          \n"
                        "veor       q9, q9          \n"
                        "veor       q10, q10        \n"
                        "veor       q11, q11        \n"
                        "veor       q12, q12        \n"
                        "veor       q13, q13        \n"
                        "veor       q14, q14        \n"
                        "veor       q15, q15        \n"

                        "0:                         \n"

                        "pld        [%2, #512]      \n"
                        "vldm       %2!, {d0-d7}    \n"

                        "pld        [%3, #512]      \n"
                        "vldm       %3!, {d8-d15}   \n"

                        "vmla.f32   q8, q4, d0[0]   \n"
                        "vmla.f32   q9, q4, d0[1]   \n"
                        "vmla.f32   q10, q4, d1[0]  \n"
                        "vmla.f32   q11, q4, d1[1]  \n"
                        "vmla.f32   q12, q4, d2[0]  \n"
                        "vmla.f32   q13, q4, d2[1]  \n"
                        "vmla.f32   q14, q4, d3[0]  \n"
                        "vmla.f32   q15, q4, d3[1]  \n"

                        "vmla.f32   q8, q5, d4[0]   \n"
                        "vmla.f32   q9, q5, d4[1]   \n"
                        "vmla.f32   q10, q5, d5[0]  \n"
                        "vmla.f32   q11, q5, d5[1]  \n"
                        "vmla.f32   q12, q5, d6[0]  \n"
                        "vmla.f32   q13, q5, d6[1]  \n"
                        "vmla.f32   q14, q5, d7[0]  \n"
                        "vmla.f32   q15, q5, d7[1]  \n"

                        "pld        [%2, #512]      \n"
                        "vldm       %2!, {d0-d7}    \n"

                        "vmla.f32   q8, q6, d0[0]   \n"
                        "vmla.f32   q9, q6, d0[1]   \n"
                        "vmla.f32   q10, q6, d1[0]  \n"
                        "vmla.f32   q11, q6, d1[1]  \n"
                        "vmla.f32   q12, q6, d2[0]  \n"
                        "vmla.f32   q13, q6, d2[1]  \n"
                        "vmla.f32   q14, q6, d3[0]  \n"
                        "vmla.f32   q15, q6, d3[1]  \n"

                        "subs       %0, %0, #1      \n"

                        "vmla.f32   q8, q7, d4[0]   \n"
                        "vmla.f32   q9, q7, d4[1]   \n"
                        "vmla.f32   q10, q7, d5[0]  \n"
                        "vmla.f32   q11, q7, d5[1]  \n"
                        "vmla.f32   q12, q7, d6[0]  \n"
                        "vmla.f32   q13, q7, d6[1]  \n"
                        "vmla.f32   q14, q7, d7[0]  \n"
                        "vmla.f32   q15, q7, d7[1]  \n"

                        "bne        0b              \n"

                        "vstm       %1!, {d16-d23}  \n"
                        "vstm       %1!, {d24-d31}  \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
                    );
#endif
                }
                for (; i+3<tiles; i+=4)
                {
#if __aarch64__
                    const float* r0 = bb2.row(i/12 + (i%12)/8 + (i%8)/4);
#else
                    const float* r0 = bb2.row(i/8 + (i%8)/4);
#endif

                    const float* k0 = kernel0_tm.row(r);

                    int nn = inch;// inch always > 0

#if __aarch64__
                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"
                        "eor    v18.16b, v18.16b, v18.16b   \n"
                        "eor    v19.16b, v19.16b, v19.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%2], #64 \n"// r0 r1 r2 r3

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%3], #64 \n"// w0123

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v8.4s, v1.s[0]      \n"
                        "fmla   v18.4s, v8.4s, v2.s[0]      \n"
                        "fmla   v19.4s, v8.4s, v3.s[0]      \n"

                        "fmla   v16.4s, v9.4s, v0.s[1]      \n"
                        "fmla   v17.4s, v9.4s, v1.s[1]      \n"
                        "fmla   v18.4s, v9.4s, v2.s[1]      \n"
                        "fmla   v19.4s, v9.4s, v3.s[1]      \n"

                        "fmla   v16.4s, v10.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v10.4s, v1.s[2]     \n"
                        "fmla   v18.4s, v10.4s, v2.s[2]     \n"
                        "fmla   v19.4s, v10.4s, v3.s[2]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v11.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v11.4s, v1.s[3]     \n"
                        "fmla   v18.4s, v11.4s, v2.s[3]     \n"
                        "fmla   v19.4s, v11.4s, v3.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%1], #64 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
                    );
#else
                    asm volatile(
                        "veor       q8, q8          \n"
                        "veor       q9, q9          \n"
                        "veor       q10, q10        \n"
                        "veor       q11, q11        \n"

                        "0:                         \n"

                        "pld        [%2, #512]      \n"
                        "vldm       %2!, {d0-d7}    \n"

                        "pld        [%3, #512]      \n"
                        "vldm       %3!, {d8-d15}   \n"

                        "vmla.f32   q8, q4, d0[0]   \n"
                        "vmla.f32   q9, q4, d2[0]   \n"
                        "vmla.f32   q10, q4, d4[0]  \n"
                        "vmla.f32   q11, q4, d6[0]  \n"

                        "vmla.f32   q8, q5, d0[1]   \n"
                        "vmla.f32   q9, q5, d2[1]   \n"
                        "vmla.f32   q10, q5, d4[1]  \n"
                        "vmla.f32   q11, q5, d6[1]  \n"

                        "vmla.f32   q8, q6, d1[0]   \n"
                        "vmla.f32   q9, q6, d3[0]   \n"
                        "vmla.f32   q10, q6, d5[0]  \n"
                        "vmla.f32   q11, q6, d7[0]  \n"

                        "subs       %0, %0, #1      \n"

                        "vmla.f32   q8, q7, d1[1]   \n"
                        "vmla.f32   q9, q7, d3[1]   \n"
                        "vmla.f32   q10, q7, d5[1]  \n"
                        "vmla.f32   q11, q7, d7[1]  \n"

                        "bne        0b              \n"

                        "vstm       %1!, {d16-d23}  \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11"
                    );
#endif
                }
                for (; i+1<tiles; i+=2)
                {
#if __aarch64__
                    const float* r0 = bb2.row(i/12 + (i%12)/8 + (i%8)/4 + (i%4)/2);
#else
                    const float* r0 = bb2.row(i/8 + (i%8)/4 + (i%4)/2);
#endif

                    const float* k0 = kernel0_tm.row(r);

                    int nn = inch;// inch always > 0

#if __aarch64__
                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"
                        "eor    v17.16b, v17.16b, v17.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%2, #256]       \n"
                        "ld1    {v0.4s, v1.4s}, [%2], #32   \n"// r0 r1

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%3], #64 \n"// w0123

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v17.4s, v8.4s, v1.s[0]      \n"

                        "fmla   v16.4s, v9.4s, v0.s[1]      \n"
                        "fmla   v17.4s, v9.4s, v1.s[1]      \n"

                        "fmla   v16.4s, v10.4s, v0.s[2]     \n"
                        "fmla   v17.4s, v10.4s, v1.s[2]     \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v11.4s, v0.s[3]     \n"
                        "fmla   v17.4s, v11.4s, v1.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s, v17.4s}, [%1], #32 \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "v0", "v1", "v8", "v9", "v10", "v11", "v16", "v17"
                    );
#else
                    asm volatile(
                        "veor       q8, q8          \n"
                        "veor       q9, q9          \n"

                        "0:                         \n"

                        "pld        [%2, #256]      \n"
                        "vld1.f32   {d0-d3}, [%2 :128]! \n"

                        "pld        [%3, #512]      \n"
                        "vldm       %3!, {d8-d15}   \n"

                        "vmla.f32   q8, q4, d0[0]   \n"
                        "vmla.f32   q9, q4, d2[0]   \n"

                        "vmla.f32   q8, q5, d0[1]   \n"
                        "vmla.f32   q9, q5, d2[1]   \n"

                        "vmla.f32   q8, q6, d1[0]   \n"
                        "vmla.f32   q9, q6, d3[0]   \n"

                        "subs       %0, %0, #1      \n"

                        "vmla.f32   q8, q7, d1[1]   \n"
                        "vmla.f32   q9, q7, d3[1]   \n"

                        "bne        0b              \n"

                        "vst1.f32   {d16-d19}, [%1 :128]! \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "q0", "q1", "q4", "q5", "q6", "q7", "q8", "q9"
                    );
#endif
                }
                for (; i<tiles; i++)
                {
#if __aarch64__
                    const float* r0 = bb2.row(i/12 + (i%12)/8 + (i%8)/4 + (i%4)/2 + i%2);
#else
                    const float* r0 = bb2.row(i/8 + (i%8)/4 + (i%4)/2 + i%2);
#endif

                    const float* k0 = kernel0_tm.row(r);

                    int nn = inch;// inch always > 0

#if __aarch64__
                    asm volatile(
                        "eor    v16.16b, v16.16b, v16.16b   \n"

                        "0:                                 \n"

                        "prfm   pldl1keep, [%2, #128]       \n"
                        "ld1    {v0.4s}, [%2], #16          \n"// r0

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%3], #64 \n"// w0123

                        "fmla   v16.4s, v8.4s, v0.s[0]      \n"
                        "fmla   v16.4s, v9.4s, v0.s[1]      \n"

                        "subs   %w0, %w0, #1                \n"

                        "fmla   v16.4s, v10.4s, v0.s[2]     \n"
                        "fmla   v16.4s, v11.4s, v0.s[3]     \n"

                        "bne    0b                          \n"

                        "st1    {v16.4s}, [%1], #16         \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "v0", "v8", "v9", "v10", "v11", "v16"
                    );
#else
                    asm volatile(
                        "veor       q8, q8          \n"

                        "0:                         \n"

                        "pld        [%2, #128]      \n"
                        "vld1.f32   {d0-d1}, [%2 :128]! \n"

                        "pld        [%3, #512]      \n"
                        "vldm       %3!, {d8-d15}   \n"

                        "vmla.f32   q8, q4, d0[0]   \n"
                        "vmla.f32   q8, q5, d0[1]   \n"

                        "subs       %0, %0, #1      \n"

                        "vmla.f32   q8, q6, d1[0]   \n"
                        "vmla.f32   q8, q7, d1[1]   \n"

                        "bne        0b              \n"

                        "vst1.f32   {d16-d17}, [%1 :128]! \n"

                        : "=r"(nn),         // %0
                          "=r"(output0_tm), // %1
                          "=r"(r0),         // %2
                          "=r"(k0)          // %3
                        : "0"(nn),
                          "1"(output0_tm),
                          "2"(r0),
                          "3"(k0)
                        : "cc", "memory", "q0", "q4", "q5", "q6", "q7", "q8"
                    );
#endif
                }

            }
        }

    }
    bottom_blob_tm = Mat();
    // END dot

    // BEGIN transform output
    Mat top_blob_bordered;
    top_blob_bordered.create(outw, outh, outch, elemsize, elempack);
    {
//         const float otm[6][8] = {
//             {1.0f,  1.0f,   1.0f,   1.0f,   1.0f,  32.0f, 32.0f, 0.0f},
//             {0.0f,  1.0f,  -1.0f,   2.0f,  -2.0f,  16.0f,-16.0f, 0.0f},
//             {0.0f,  1.0f,   1.0f,   4.0f,   4.0f,   8.0f,  8.0f, 0.0f},
//             {0.0f,  1.0f,  -1.0f,   8.0f,  -8.0f,   4.0f, -4.0f, 0.0f},
//             {0.0f,  1.0f,   1.0f,  16.0f,  16.0f,   2.0f,  2.0f, 0.0f},
//             {0.0f,  1.0f,  -1.0f,  32.0f, -32.0f,   1.0f, -1.0f, 1.0f}
//         };

        // 0 = r0 + (r1 + r2) + (r3 + r4)     + (r5 + r6) * 32
        // 1 =      (r1 - r2) + (r3 - r4) * 2 + (r5 - r6) * 16
        // 2 =      (r1 + r2) + (r3 + r4) * 4 + (r5 + r6) * 8
        // 3 =      (r1 - r2) + (r3 - r4) * 8 + (r5 - r6) * 4
        // 4 =      (r1 + r2) + (r3 + r4) * 16+ (r5 + r6) * 2
        // 5 = r7 + (r1 - r2) + (r3 - r4) * 32+ (r5 - r6)

        int w_tm = outw / 6 * 8;
        int h_tm = outh / 6 * 8;
        const int tiles = w_tm/8 * h_tm/8;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p<outch; p++)
        {
            const Mat out0_tm = top_blob_tm.channel(p);
            Mat out0 = top_blob_bordered.channel(p);

//             const float bias0 = bias ? bias[p] : 0.f;
            float32x4_t _bias0 = bias ? vld1q_f32( (const float*)bias + p * 4) : vdupq_n_f32(0.f);

            float tmp[6][8][4];

            // tile
            for (int i=0; i<outh/6; i++)
            {
                for (int j=0; j<outw/6; j++)
                {
//                     top_blob_tm.create(tiles, 64, outch, elemsize, elempack);

                    const float* output0_tm_0 = (const float*)out0_tm + (i * w_tm/8 + j) * 4;
                    const float* output0_tm_1 = output0_tm_0 + tiles * 4;
                    const float* output0_tm_2 = output0_tm_0 + tiles * 8;
                    const float* output0_tm_3 = output0_tm_0 + tiles * 12;
                    const float* output0_tm_4 = output0_tm_0 + tiles * 16;
                    const float* output0_tm_5 = output0_tm_0 + tiles * 20;
                    const float* output0_tm_6 = output0_tm_0 + tiles * 24;
                    const float* output0_tm_7 = output0_tm_0 + tiles * 28;

                    float* output0 = out0.row(i * 6) + (j * 6) * 4;

                    // TODO neon optimize
                    for (int m=0; m<8; m++)
                    {
                        float32x4_t _out0tm0 = vld1q_f32(output0_tm_0);
                        float32x4_t _out0tm1 = vld1q_f32(output0_tm_1);
                        float32x4_t _out0tm2 = vld1q_f32(output0_tm_2);
                        float32x4_t _out0tm3 = vld1q_f32(output0_tm_3);
                        float32x4_t _out0tm4 = vld1q_f32(output0_tm_4);
                        float32x4_t _out0tm5 = vld1q_f32(output0_tm_5);
                        float32x4_t _out0tm6 = vld1q_f32(output0_tm_6);
                        float32x4_t _out0tm7 = vld1q_f32(output0_tm_7);

                        float32x4_t _tmp024a = vaddq_f32(_out0tm1, _out0tm2);
                        float32x4_t _tmp135a = vsubq_f32(_out0tm1, _out0tm2);

//                         float tmp024a = output0_tm[1] + output0_tm[2];
//                         float tmp135a = output0_tm[1] - output0_tm[2];

                        float32x4_t _tmp024b = vaddq_f32(_out0tm3, _out0tm4);
                        float32x4_t _tmp135b = vsubq_f32(_out0tm3, _out0tm4);

//                         float tmp024b = output0_tm[3] + output0_tm[4];
//                         float tmp135b = output0_tm[3] - output0_tm[4];

                        float32x4_t _tmp024c = vaddq_f32(_out0tm5, _out0tm6);
                        float32x4_t _tmp135c = vsubq_f32(_out0tm5, _out0tm6);

//                         float tmp024c = output0_tm[5] + output0_tm[6];
//                         float tmp135c = output0_tm[5] - output0_tm[6];

                        float32x4_t _tmp0m = vaddq_f32(vaddq_f32(_out0tm0, _tmp024a), vmlaq_n_f32(_tmp024b, _tmp024c, 32.f));
                        float32x4_t _tmp2m = vmlaq_n_f32(vmlaq_n_f32(_tmp024a, _tmp024b, 4.f), _tmp024c, 8.f);
                        float32x4_t _tmp4m = vmlaq_n_f32(vmlaq_n_f32(_tmp024a, _tmp024b, 16.f), _tmp024c, 2.f);
                        vst1q_f32(tmp[0][m], _tmp0m);
                        vst1q_f32(tmp[2][m], _tmp2m);
                        vst1q_f32(tmp[4][m], _tmp4m);

//                         tmp[0][m] = output0_tm[0] + tmp024a + tmp024b + tmp024c * 32;
//                         tmp[2][m] = tmp024a + tmp024b * 4 + tmp024c * 8;
//                         tmp[4][m] = tmp024a + tmp024b * 16 + tmp024c + tmp024c;

                        float32x4_t _tmp1m = vmlaq_n_f32(vmlaq_n_f32(_tmp135a, _tmp135b, 2.f), _tmp135c, 16.f);
                        float32x4_t _tmp3m = vmlaq_n_f32(vmlaq_n_f32(_tmp135a, _tmp135b, 8.f), _tmp135c, 4.f);
                        float32x4_t _tmp5m = vaddq_f32(vaddq_f32(_out0tm7, _tmp135a), vmlaq_n_f32(_tmp135c, _tmp135b, 32.f));
                        vst1q_f32(tmp[1][m], _tmp1m);
                        vst1q_f32(tmp[3][m], _tmp3m);
                        vst1q_f32(tmp[5][m], _tmp5m);

//                         tmp[1][m] = tmp135a + tmp135b + tmp135b + tmp135c * 16;
//                         tmp[3][m] = tmp135a + tmp135b * 8 + tmp135c * 4;
//                         tmp[5][m] = output0_tm[7] + tmp135a + tmp135b * 32 + tmp135c;

                        output0_tm_0 += tiles * 32;
                        output0_tm_1 += tiles * 32;
                        output0_tm_2 += tiles * 32;
                        output0_tm_3 += tiles * 32;
                        output0_tm_4 += tiles * 32;
                        output0_tm_5 += tiles * 32;
                        output0_tm_6 += tiles * 32;
                        output0_tm_7 += tiles * 32;
                    }

                    for (int m=0; m<6; m++)
                    {
                        float32x4_t _tmp00 = vld1q_f32(tmp[m][0]);
                        float32x4_t _tmp01 = vld1q_f32(tmp[m][1]);
                        float32x4_t _tmp02 = vld1q_f32(tmp[m][2]);
                        float32x4_t _tmp03 = vld1q_f32(tmp[m][3]);
                        float32x4_t _tmp04 = vld1q_f32(tmp[m][4]);
                        float32x4_t _tmp05 = vld1q_f32(tmp[m][5]);
                        float32x4_t _tmp06 = vld1q_f32(tmp[m][6]);
                        float32x4_t _tmp07 = vld1q_f32(tmp[m][7]);

                        float32x4_t _tmp024a = vaddq_f32(_tmp01, _tmp02);
                        float32x4_t _tmp135a = vsubq_f32(_tmp01, _tmp02);

//                         float tmp024a = tmp0[1] + tmp0[2];
//                         float tmp135a = tmp0[1] - tmp0[2];

                        float32x4_t _tmp024b = vaddq_f32(_tmp03, _tmp04);
                        float32x4_t _tmp135b = vsubq_f32(_tmp03, _tmp04);

//                         float tmp024b = tmp0[3] + tmp0[4];
//                         float tmp135b = tmp0[3] - tmp0[4];

                        float32x4_t _tmp024c = vaddq_f32(_tmp05, _tmp06);
                        float32x4_t _tmp135c = vsubq_f32(_tmp05, _tmp06);

//                         float tmp024c = tmp0[5] + tmp0[6];
//                         float tmp135c = tmp0[5] - tmp0[6];

                        float32x4_t _out00 = vaddq_f32(_bias0, vaddq_f32(vaddq_f32(_tmp00, _tmp024a), vmlaq_n_f32(_tmp024b, _tmp024c, 32.f)));
                        float32x4_t _out02 = vaddq_f32(_bias0, vmlaq_n_f32(vmlaq_n_f32(_tmp024a, _tmp024b, 4.f), _tmp024c, 8.f));
                        float32x4_t _out04 = vaddq_f32(_bias0, vmlaq_n_f32(vmlaq_n_f32(_tmp024a, _tmp024b, 16.f), _tmp024c, 2.f));
                        vst1q_f32(output0, _out00);
                        vst1q_f32(output0 + 8, _out02);
                        vst1q_f32(output0 + 16, _out04);

//                         output0[0] = bias0 + tmp0[0] + tmp024a + tmp024b + tmp024c * 32;
//                         output0[2] = bias0 + tmp024a + tmp024b * 4 + tmp024c * 8;
//                         output0[4] = bias0 + tmp024a + tmp024b * 16 + tmp024c + tmp024c;

                        float32x4_t _out01 = vaddq_f32(_bias0, vmlaq_n_f32(vmlaq_n_f32(_tmp135a, _tmp135b, 2.f), _tmp135c, 16.f));
                        float32x4_t _out03 = vaddq_f32(_bias0, vmlaq_n_f32(vmlaq_n_f32(_tmp135a, _tmp135b, 8.f), _tmp135c, 4.f));
                        float32x4_t _out05 = vaddq_f32(_bias0, vaddq_f32(vaddq_f32(_tmp07, _tmp135a), vmlaq_n_f32(_tmp135c, _tmp135b, 32.f)));
                        vst1q_f32(output0 + 4, _out01);
                        vst1q_f32(output0 + 12, _out03);
                        vst1q_f32(output0 + 20, _out05);

//                         output0[1] = bias0 + tmp135a + tmp135b + tmp135b + tmp135c * 16;
//                         output0[3] = bias0 + tmp135a + tmp135b * 8 + tmp135c * 4;
//                         output0[5] = bias0 + tmp0[7] + tmp135a + tmp135b * 32 + tmp135c;

                        output0 += outw * 4;
                    }
                }
            }
        }
    }
    // END transform output

    // cut result pad
    copy_cut_border(top_blob_bordered, top_blob, 0, top_blob_bordered.h - top_blob.h, 0, top_blob_bordered.w - top_blob.w, opt);
}

static void conv3x3s2_pack4_neon(const Mat& bottom_blob, Mat& top_blob, const Mat& kernel, const Mat& _bias, const Option& opt)
{
    int w = bottom_blob.w;
    int inch = bottom_blob.c;
    int outw = top_blob.w;
    int outh = top_blob.h;
    int outch = top_blob.c;

    const int tailstep = (w - 2*outw + w) * 4;

    const float* bias = _bias;

    #pragma omp parallel for num_threads(opt.num_threads)
    for (int p=0; p<outch; p++)
    {
        Mat out0 = top_blob.channel(p);

        float32x4_t _bias0 = bias ? vld1q_f32((const float*)bias + p * 4) : vdupq_n_f32(0.f);
        out0.fill(_bias0);

        for (int q=0; q<inch; q++)
        {
            float* outptr0 = out0.row(0);

            const Mat img0 = bottom_blob.channel(q);

            const float* r0 = img0.row(0);
            const float* r1 = img0.row(1);
            const float* r2 = img0.row(2);

            const float* kptr = (const float*)kernel.channel(p).row(q);

            int i = 0;
            for (; i < outh; i++)
            {
                int j = 0;
                for (; j+3<outw; j+=4)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #512]       \n"
                        "ld1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%0] \n"// sum0 sum1 sum2 sum3

                        "prfm   pldl1keep, [%1, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%1], #64 \n"// r00 r01 r02 r03

                        "prfm   pldl1keep, [%1, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%1], #64 \n"// r04 r05 r06 r07

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v16.4s, v0.s[0]     \n"
                        "fmla   v21.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v22.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v6.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v0.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v22.4s, v17.4s, v4.s[1]     \n"
                        "fmla   v23.4s, v17.4s, v6.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v18.4s, v0.s[2]     \n"
                        "fmla   v21.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v22.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v6.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v0.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v2.s[3]     \n"
                        "fmla   v22.4s, v19.4s, v4.s[3]     \n"
                        "fmla   v23.4s, v19.4s, v6.s[3]     \n"

                        "prfm   pldl1keep, [%1, #128]       \n"
                        "ld1    {v28.4s}, [%1]              \n"// r08

                        "fmla   v20.4s, v24.4s, v1.s[0]     \n"
                        "fmla   v21.4s, v24.4s, v3.s[0]     \n"
                        "fmla   v22.4s, v24.4s, v5.s[0]     \n"
                        "fmla   v23.4s, v24.4s, v7.s[0]     \n"
                        "fmla   v20.4s, v25.4s, v1.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v3.s[1]     \n"
                        "fmla   v22.4s, v25.4s, v5.s[1]     \n"
                        "fmla   v23.4s, v25.4s, v7.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v26.4s, v1.s[2]     \n"
                        "fmla   v21.4s, v26.4s, v3.s[2]     \n"
                        "fmla   v22.4s, v26.4s, v5.s[2]     \n"
                        "fmla   v23.4s, v26.4s, v7.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v1.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v3.s[3]     \n"
                        "fmla   v22.4s, v27.4s, v5.s[3]     \n"
                        "fmla   v23.4s, v27.4s, v7.s[3]     \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v8.4s, v9.4s, v10.4s, v11.4s}, [%2], #64 \n"// r10 r11 r12 r13

                        "fmla   v20.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v21.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v22.4s, v16.4s, v6.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v28.s[0]    \n"
                        "fmla   v20.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v4.s[1]     \n"
                        "fmla   v22.4s, v17.4s, v6.s[1]     \n"
                        "fmla   v23.4s, v17.4s, v28.s[1]    \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v21.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v22.4s, v18.4s, v6.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v28.s[2]    \n"
                        "fmla   v20.4s, v19.4s, v2.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v4.s[3]     \n"
                        "fmla   v22.4s, v19.4s, v6.s[3]     \n"
                        "fmla   v23.4s, v19.4s, v28.s[3]    \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v12.4s, v13.4s, v14.4s, v15.4s}, [%2], #64 \n"// r14 r15 r16 r17

                        "fmla   v20.4s, v24.4s, v8.s[0]     \n"
                        "fmla   v21.4s, v24.4s, v10.s[0]    \n"
                        "fmla   v22.4s, v24.4s, v12.s[0]    \n"
                        "fmla   v23.4s, v24.4s, v14.s[0]    \n"
                        "fmla   v20.4s, v25.4s, v8.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v10.s[1]    \n"
                        "fmla   v22.4s, v25.4s, v12.s[1]    \n"
                        "fmla   v23.4s, v25.4s, v14.s[1]    \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v26.4s, v8.s[2]     \n"
                        "fmla   v21.4s, v26.4s, v10.s[2]    \n"
                        "fmla   v22.4s, v26.4s, v12.s[2]    \n"
                        "fmla   v23.4s, v26.4s, v14.s[2]    \n"
                        "fmla   v20.4s, v27.4s, v8.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v10.s[3]    \n"
                        "fmla   v22.4s, v27.4s, v12.s[3]    \n"
                        "fmla   v23.4s, v27.4s, v14.s[3]    \n"

                        "prfm   pldl1keep, [%2, #128]       \n"
                        "ld1    {v28.4s}, [%2]              \n"// r18

                        "fmla   v20.4s, v16.4s, v9.s[0]     \n"
                        "fmla   v21.4s, v16.4s, v11.s[0]    \n"
                        "fmla   v22.4s, v16.4s, v13.s[0]    \n"
                        "fmla   v23.4s, v16.4s, v15.s[0]    \n"
                        "fmla   v20.4s, v17.4s, v9.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v11.s[1]    \n"
                        "fmla   v22.4s, v17.4s, v13.s[1]    \n"
                        "fmla   v23.4s, v17.4s, v15.s[1]    \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v18.4s, v9.s[2]     \n"
                        "fmla   v21.4s, v18.4s, v11.s[2]    \n"
                        "fmla   v22.4s, v18.4s, v13.s[2]    \n"
                        "fmla   v23.4s, v18.4s, v15.s[2]    \n"
                        "fmla   v20.4s, v19.4s, v9.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v11.s[3]    \n"
                        "fmla   v22.4s, v19.4s, v13.s[3]    \n"
                        "fmla   v23.4s, v19.4s, v15.s[3]    \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"// r20 r21 r22 r23

                        "fmla   v20.4s, v24.4s, v10.s[0]    \n"
                        "fmla   v21.4s, v24.4s, v12.s[0]    \n"
                        "fmla   v22.4s, v24.4s, v14.s[0]    \n"
                        "fmla   v23.4s, v24.4s, v28.s[0]    \n"
                        "fmla   v20.4s, v25.4s, v10.s[1]    \n"
                        "fmla   v21.4s, v25.4s, v12.s[1]    \n"
                        "fmla   v22.4s, v25.4s, v14.s[1]    \n"
                        "fmla   v23.4s, v25.4s, v28.s[1]    \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v26.4s, v10.s[2]    \n"
                        "fmla   v21.4s, v26.4s, v12.s[2]    \n"
                        "fmla   v22.4s, v26.4s, v14.s[2]    \n"
                        "fmla   v23.4s, v26.4s, v28.s[2]    \n"
                        "fmla   v20.4s, v27.4s, v10.s[3]    \n"
                        "fmla   v21.4s, v27.4s, v12.s[3]    \n"
                        "fmla   v22.4s, v27.4s, v14.s[3]    \n"
                        "fmla   v23.4s, v27.4s, v28.s[3]    \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v4.4s, v5.4s, v6.4s, v7.4s}, [%3], #64 \n"// r24 r25 r26 r27

                        "fmla   v20.4s, v16.4s, v0.s[0]     \n"
                        "fmla   v21.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v22.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v6.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v0.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v22.4s, v17.4s, v4.s[1]     \n"
                        "fmla   v23.4s, v17.4s, v6.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v20.4s, v18.4s, v0.s[2]     \n"
                        "fmla   v21.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v22.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v6.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v0.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v2.s[3]     \n"
                        "fmla   v22.4s, v19.4s, v4.s[3]     \n"
                        "fmla   v23.4s, v19.4s, v6.s[3]     \n"

                        "prfm   pldl1keep, [%3, #128]       \n"
                        "ld1    {v28.4s}, [%3]              \n"// r28

                        "fmla   v20.4s, v24.4s, v1.s[0]     \n"
                        "fmla   v21.4s, v24.4s, v3.s[0]     \n"
                        "fmla   v22.4s, v24.4s, v5.s[0]     \n"
                        "fmla   v23.4s, v24.4s, v7.s[0]     \n"
                        "fmla   v20.4s, v25.4s, v1.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v3.s[1]     \n"
                        "fmla   v22.4s, v25.4s, v5.s[1]     \n"
                        "fmla   v23.4s, v25.4s, v7.s[1]     \n"

//                         "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4] \n"

                        "fmla   v20.4s, v26.4s, v1.s[2]     \n"
                        "fmla   v21.4s, v26.4s, v3.s[2]     \n"
                        "fmla   v22.4s, v26.4s, v5.s[2]     \n"
                        "fmla   v23.4s, v26.4s, v7.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v1.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v3.s[3]     \n"
                        "fmla   v22.4s, v27.4s, v5.s[3]     \n"
                        "fmla   v23.4s, v27.4s, v7.s[3]     \n"

                        "fmla   v20.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v21.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v22.4s, v16.4s, v6.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v28.s[0]    \n"
                        "fmla   v20.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v4.s[1]     \n"
                        "fmla   v22.4s, v17.4s, v6.s[1]     \n"
                        "fmla   v23.4s, v17.4s, v28.s[1]    \n"
                        "fmla   v20.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v21.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v22.4s, v18.4s, v6.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v28.s[2]    \n"
                        "fmla   v20.4s, v19.4s, v2.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v4.s[3]     \n"
                        "fmla   v22.4s, v19.4s, v6.s[3]     \n"
                        "fmla   v23.4s, v19.4s, v28.s[3]    \n"

                        "sub    %4, %4, #512                \n"// kptr -= 8 * 16;

                        "st1    {v20.4s, v21.4s, v22.4s, v23.4s}, [%0], #64 \n"

                        : "=r"(outptr0),    // %0
                          "=r"(r0),         // %1
                          "=r"(r1),         // %2
                          "=r"(r2),         // %3
                          "=r"(kptr)        // %4
                        : "0"(outptr0),
                          "1"(r0),
                          "2"(r1),
                          "3"(r2),
                          "4"(kptr)
                        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28"
                    );
#else // __aarch64__
                    asm volatile(
                        "pld        [%0, #512]          \n"
                        "vldm       %0, {d24-d31}       \n"// sum0 sum1 sum2 sum3

                        "pld        [%1, #512]          \n"
                        "vldm       %1!, {d0-d7}        \n"// r00 r01 r02 r03

                        "pld        [%1, #512]          \n"
                        "vldm       %1!, {d8-d15}       \n"// r04 r05 r06 r07

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q12, q8, d0[0]      \n"
                        "vmla.f32   q13, q8, d4[0]      \n"
                        "vmla.f32   q14, q8, d8[0]      \n"
                        "vmla.f32   q15, q8, d12[0]     \n"
                        "vmla.f32   q12, q9, d0[1]      \n"
                        "vmla.f32   q13, q9, d4[1]      \n"
                        "vmla.f32   q14, q9, d8[1]      \n"
                        "vmla.f32   q15, q9, d12[1]     \n"
                        "vmla.f32   q12, q10, d1[0]     \n"
                        "vmla.f32   q13, q10, d5[0]     \n"
                        "vmla.f32   q14, q10, d9[0]     \n"
                        "vmla.f32   q15, q10, d13[0]    \n"
                        "vmla.f32   q12, q11, d1[1]     \n"
                        "vmla.f32   q13, q11, d5[1]     \n"
                        "vmla.f32   q14, q11, d9[1]     \n"
                        "vmla.f32   q15, q11, d13[1]    \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "pld        [%1, #128]          \n"
                        "vld1.f32   {d0-d1}, [%1 :128]  \n"// r08

                        "vmla.f32   q12, q8, d2[0]      \n"
                        "vmla.f32   q13, q8, d6[0]      \n"
                        "vmla.f32   q14, q8, d10[0]     \n"
                        "vmla.f32   q15, q8, d14[0]     \n"
                        "vmla.f32   q12, q9, d2[1]      \n"
                        "vmla.f32   q13, q9, d6[1]      \n"
                        "vmla.f32   q14, q9, d10[1]     \n"
                        "vmla.f32   q15, q9, d14[1]     \n"
                        "vmla.f32   q12, q10, d3[0]     \n"
                        "vmla.f32   q13, q10, d7[0]     \n"
                        "vmla.f32   q14, q10, d11[0]    \n"
                        "vmla.f32   q15, q10, d15[0]    \n"
                        "vmla.f32   q12, q11, d3[1]     \n"
                        "vmla.f32   q13, q11, d7[1]     \n"
                        "vmla.f32   q14, q11, d11[1]    \n"
                        "vmla.f32   q15, q11, d15[1]    \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q12, q8, d4[0]      \n"
                        "vmla.f32   q13, q8, d8[0]      \n"
                        "vmla.f32   q14, q8, d12[0]     \n"
                        "vmla.f32   q15, q8, d0[0]      \n"
                        "vmla.f32   q12, q9, d4[1]      \n"
                        "vmla.f32   q13, q9, d8[1]      \n"
                        "vmla.f32   q14, q9, d12[1]     \n"
                        "vmla.f32   q15, q9, d0[1]      \n"
                        "vmla.f32   q12, q10, d5[0]     \n"
                        "vmla.f32   q13, q10, d9[0]     \n"
                        "vmla.f32   q14, q10, d13[0]    \n"
                        "vmla.f32   q15, q10, d1[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"
                        "vmla.f32   q13, q11, d9[1]     \n"
                        "vmla.f32   q14, q11, d13[1]    \n"
                        "vmla.f32   q15, q11, d1[1]     \n"

                        "pld        [%2, #512]          \n"
                        "vldm       %2!, {d8-d15}       \n"// r10 r11 r12 r13

                        "pld        [%2, #512]          \n"
                        "vldm       %2!, {d0-d7}        \n"// r14 r15 r16 r17

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q12, q8, d8[0]      \n"
                        "vmla.f32   q13, q8, d12[0]     \n"
                        "vmla.f32   q14, q8, d0[0]      \n"
                        "vmla.f32   q15, q8, d4[0]      \n"
                        "vmla.f32   q12, q9, d8[1]      \n"
                        "vmla.f32   q13, q9, d12[1]     \n"
                        "vmla.f32   q14, q9, d0[1]      \n"
                        "vmla.f32   q15, q9, d4[1]      \n"
                        "vmla.f32   q12, q10, d9[0]     \n"
                        "vmla.f32   q13, q10, d13[0]    \n"
                        "vmla.f32   q14, q10, d1[0]     \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d9[1]     \n"
                        "vmla.f32   q13, q11, d13[1]    \n"
                        "vmla.f32   q14, q11, d1[1]     \n"
                        "vmla.f32   q15, q11, d5[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "pld        [%2, #128]          \n"
                        "vld1.f32   {d8-d9}, [%2 :128]  \n"// r18

                        "vmla.f32   q12, q8, d10[0]     \n"
                        "vmla.f32   q13, q8, d14[0]     \n"
                        "vmla.f32   q14, q8, d2[0]      \n"
                        "vmla.f32   q15, q8, d6[0]      \n"
                        "vmla.f32   q12, q9, d10[1]     \n"
                        "vmla.f32   q13, q9, d14[1]     \n"
                        "vmla.f32   q14, q9, d2[1]      \n"
                        "vmla.f32   q15, q9, d6[1]      \n"
                        "vmla.f32   q12, q10, d11[0]    \n"
                        "vmla.f32   q13, q10, d15[0]    \n"
                        "vmla.f32   q14, q10, d3[0]     \n"
                        "vmla.f32   q15, q10, d7[0]     \n"
                        "vmla.f32   q12, q11, d11[1]    \n"
                        "vmla.f32   q13, q11, d15[1]    \n"
                        "vmla.f32   q14, q11, d3[1]     \n"
                        "vmla.f32   q15, q11, d7[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q12, q8, d12[0]     \n"
                        "vmla.f32   q13, q8, d0[0]      \n"
                        "vmla.f32   q14, q8, d4[0]      \n"
                        "vmla.f32   q15, q8, d8[0]      \n"
                        "vmla.f32   q12, q9, d12[1]     \n"
                        "vmla.f32   q13, q9, d0[1]      \n"
                        "vmla.f32   q14, q9, d4[1]      \n"
                        "vmla.f32   q15, q9, d8[1]      \n"
                        "vmla.f32   q12, q10, d13[0]    \n"
                        "vmla.f32   q13, q10, d1[0]     \n"
                        "vmla.f32   q14, q10, d5[0]     \n"
                        "vmla.f32   q15, q10, d9[0]     \n"
                        "vmla.f32   q12, q11, d13[1]    \n"
                        "vmla.f32   q13, q11, d1[1]     \n"
                        "vmla.f32   q14, q11, d5[1]     \n"
                        "vmla.f32   q15, q11, d9[1]     \n"

                        "pld        [%3, #512]          \n"
                        "vldm       %3!, {d0-d7}        \n"// r20 r21 r22 r23

                        "pld        [%3, #512]          \n"
                        "vldm       %3!, {d8-d15}       \n"// r24 r25 r26 r27

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q12, q8, d0[0]      \n"
                        "vmla.f32   q13, q8, d4[0]      \n"
                        "vmla.f32   q14, q8, d8[0]      \n"
                        "vmla.f32   q15, q8, d12[0]     \n"
                        "vmla.f32   q12, q9, d0[1]      \n"
                        "vmla.f32   q13, q9, d4[1]      \n"
                        "vmla.f32   q14, q9, d8[1]      \n"
                        "vmla.f32   q15, q9, d12[1]     \n"
                        "vmla.f32   q12, q10, d1[0]     \n"
                        "vmla.f32   q13, q10, d5[0]     \n"
                        "vmla.f32   q14, q10, d9[0]     \n"
                        "vmla.f32   q15, q10, d13[0]    \n"
                        "vmla.f32   q12, q11, d1[1]     \n"
                        "vmla.f32   q13, q11, d5[1]     \n"
                        "vmla.f32   q14, q11, d9[1]     \n"
                        "vmla.f32   q15, q11, d13[1]    \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "pld        [%3, #128]          \n"
                        "vld1.f32   {d0-d1}, [%3 :128]  \n"// r28

                        "vmla.f32   q12, q8, d2[0]      \n"
                        "vmla.f32   q13, q8, d6[0]      \n"
                        "vmla.f32   q14, q8, d10[0]     \n"
                        "vmla.f32   q15, q8, d14[0]     \n"
                        "vmla.f32   q12, q9, d2[1]      \n"
                        "vmla.f32   q13, q9, d6[1]      \n"
                        "vmla.f32   q14, q9, d10[1]     \n"
                        "vmla.f32   q15, q9, d14[1]     \n"
                        "vmla.f32   q12, q10, d3[0]     \n"
                        "vmla.f32   q13, q10, d7[0]     \n"
                        "vmla.f32   q14, q10, d11[0]    \n"
                        "vmla.f32   q15, q10, d15[0]    \n"
                        "vmla.f32   q12, q11, d3[1]     \n"
                        "vmla.f32   q13, q11, d7[1]     \n"
                        "vmla.f32   q14, q11, d11[1]    \n"
                        "vmla.f32   q15, q11, d15[1]    \n"

//                         "pld        [%4, #512]          \n"
                        "vldm       %4, {d16-d23}       \n"

                        "vmla.f32   q12, q8, d4[0]      \n"
                        "vmla.f32   q13, q8, d8[0]      \n"
                        "vmla.f32   q14, q8, d12[0]     \n"
                        "vmla.f32   q15, q8, d0[0]      \n"
                        "vmla.f32   q12, q9, d4[1]      \n"
                        "vmla.f32   q13, q9, d8[1]      \n"
                        "vmla.f32   q14, q9, d12[1]     \n"
                        "vmla.f32   q15, q9, d0[1]      \n"
                        "vmla.f32   q12, q10, d5[0]     \n"
                        "vmla.f32   q13, q10, d9[0]     \n"
                        "vmla.f32   q14, q10, d13[0]    \n"
                        "vmla.f32   q15, q10, d1[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"
                        "vmla.f32   q13, q11, d9[1]     \n"
                        "vmla.f32   q14, q11, d13[1]    \n"
                        "vmla.f32   q15, q11, d1[1]     \n"

                        "sub        %4, %4, #512        \n"// kptr -= 8 * 16;

                        "vstm       %0!, {d24-d31}      \n"

                        : "=r"(outptr0),    // %0
                          "=r"(r0),         // %1
                          "=r"(r1),         // %2
                          "=r"(r2),         // %3
                          "=r"(kptr)        // %4
                        : "0"(outptr0),
                          "1"(r0),
                          "2"(r1),
                          "3"(r2),
                          "4"(kptr)
                        : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
                    );
#endif // __aarch64__
                }
                for (; j+1<outw; j+=2)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #256]       \n"
                        "ld1    {v20.4s, v21.4s}, [%0]      \n"// sum0 sum1

                        "prfm   pldl1keep, [%1, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%1], #64 \n"// r00 r01 r02 r03

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmul   v22.4s, v16.4s, v0.s[0]     \n"
                        "fmul   v23.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v0.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v2.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v18.4s, v0.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v0.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v2.s[3]     \n"

                        "prfm   pldl1keep, [%1, #128]       \n"
                        "ld1    {v4.4s}, [%1]               \n"// r04

                        "fmla   v22.4s, v24.4s, v1.s[0]     \n"
                        "fmla   v23.4s, v24.4s, v3.s[0]     \n"
                        "fmla   v20.4s, v25.4s, v1.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v3.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v26.4s, v1.s[2]     \n"
                        "fmla   v23.4s, v26.4s, v3.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v1.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v3.s[3]     \n"

                        "fmla   v22.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v4.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v2.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v4.s[3]     \n"

                        "prfm   pldl1keep, [%2, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%2], #64 \n"// r10 r11 r12 r13

                        "fmla   v22.4s, v24.4s, v0.s[0]     \n"
                        "fmla   v23.4s, v24.4s, v2.s[0]     \n"
                        "fmla   v20.4s, v25.4s, v0.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v2.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v26.4s, v0.s[2]     \n"
                        "fmla   v23.4s, v26.4s, v2.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v0.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v2.s[3]     \n"

                        "prfm   pldl1keep, [%2, #128]       \n"
                        "ld1    {v4.4s}, [%2]               \n"// r14

                        "fmla   v22.4s, v16.4s, v1.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v3.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v1.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v3.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v18.4s, v1.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v3.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v1.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v3.s[3]     \n"

                        "fmla   v22.4s, v24.4s, v2.s[0]     \n"
                        "fmla   v23.4s, v24.4s, v4.s[0]     \n"
                        "fmla   v20.4s, v25.4s, v2.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v4.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v26.4s, v2.s[2]     \n"
                        "fmla   v23.4s, v26.4s, v4.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v2.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v4.s[3]     \n"

                        "prfm   pldl1keep, [%3, #512]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s, v3.4s}, [%3], #64 \n"// r20 r21 r22 r23

                        "fmla   v22.4s, v16.4s, v0.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v0.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v2.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v22.4s, v18.4s, v0.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v0.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v2.s[3]     \n"

                        "prfm   pldl1keep, [%3, #128]       \n"
                        "ld1    {v4.4s}, [%3]               \n"// r24

                        "fmla   v22.4s, v24.4s, v1.s[0]     \n"
                        "fmla   v23.4s, v24.4s, v3.s[0]     \n"
                        "fmla   v20.4s, v25.4s, v1.s[1]     \n"
                        "fmla   v21.4s, v25.4s, v3.s[1]     \n"

//                         "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4] \n"

                        "fmla   v22.4s, v26.4s, v1.s[2]     \n"
                        "fmla   v23.4s, v26.4s, v3.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v1.s[3]     \n"
                        "fmla   v21.4s, v27.4s, v3.s[3]     \n"

                        "fmla   v22.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v23.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v20.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v21.4s, v17.4s, v4.s[1]     \n"
                        "fmla   v22.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v23.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v2.s[3]     \n"
                        "fmla   v21.4s, v19.4s, v4.s[3]     \n"

                        "fadd   v20.4s, v20.4s, v22.4s      \n"
                        "fadd   v21.4s, v21.4s, v23.4s      \n"

                        "sub    %4, %4, #512                \n"// kptr -= 8 * 16;

                        "st1    {v20.4s, v21.4s}, [%0], #32 \n"

                        : "=r"(outptr0),    // %0
                          "=r"(r0),         // %1
                          "=r"(r1),         // %2
                          "=r"(r2),         // %3
                          "=r"(kptr)        // %4
                        : "0"(outptr0),
                          "1"(r0),
                          "2"(r1),
                          "3"(r2),
                          "4"(kptr)
                        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27"
                    );
#else // __aarch64__
                    asm volatile(
                        "pld        [%0, #256]          \n"
                        "vld1.f32   {d24-d27}, [%0 :128] \n"// sum0 sum1

                        "pld        [%1, #512]          \n"
                        "vldm       %1!, {d0-d7}        \n"// r00 r01 r02 r03

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmul.f32   q14, q8, d0[0]      \n"
                        "vmul.f32   q15, q8, d4[0]      \n"
                        "vmla.f32   q12, q9, d0[1]      \n"
                        "vmla.f32   q13, q9, d4[1]      \n"
                        "vmla.f32   q14, q10, d1[0]     \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d1[1]     \n"
                        "vmla.f32   q13, q11, d5[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "pld        [%1, #128]          \n"
                        "vld1.f32   {d8-d9}, [%1 :128]  \n"// r04

                        "vmla.f32   q14, q8, d2[0]      \n"
                        "vmla.f32   q15, q8, d6[0]      \n"
                        "vmla.f32   q12, q9, d2[1]      \n"
                        "vmla.f32   q13, q9, d6[1]      \n"
                        "vmla.f32   q14, q10, d3[0]     \n"
                        "vmla.f32   q15, q10, d7[0]     \n"
                        "vmla.f32   q12, q11, d3[1]     \n"
                        "vmla.f32   q13, q11, d7[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q14, q8, d4[0]      \n"
                        "vmla.f32   q15, q8, d8[0]      \n"
                        "vmla.f32   q12, q9, d4[1]      \n"
                        "vmla.f32   q13, q9, d8[1]      \n"
                        "vmla.f32   q14, q10, d5[0]     \n"
                        "vmla.f32   q15, q10, d9[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"
                        "vmla.f32   q13, q11, d9[1]     \n"

                        "pld        [%2, #512]          \n"
                        "vldm       %2!, {d0-d7}        \n"// r10 r11 r12 r13

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q14, q8, d0[0]      \n"
                        "vmla.f32   q15, q8, d4[0]      \n"
                        "vmla.f32   q12, q9, d0[1]      \n"
                        "vmla.f32   q13, q9, d4[1]      \n"
                        "vmla.f32   q14, q10, d1[0]     \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d1[1]     \n"
                        "vmla.f32   q13, q11, d5[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "pld        [%2, #128]          \n"
                        "vld1.f32   {d8-d9}, [%2 :128]  \n"// r14

                        "vmla.f32   q14, q8, d2[0]      \n"
                        "vmla.f32   q15, q8, d6[0]      \n"
                        "vmla.f32   q12, q9, d2[1]      \n"
                        "vmla.f32   q13, q9, d6[1]      \n"
                        "vmla.f32   q14, q10, d3[0]     \n"
                        "vmla.f32   q15, q10, d7[0]     \n"
                        "vmla.f32   q12, q11, d3[1]     \n"
                        "vmla.f32   q13, q11, d7[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q14, q8, d4[0]      \n"
                        "vmla.f32   q15, q8, d8[0]      \n"
                        "vmla.f32   q12, q9, d4[1]      \n"
                        "vmla.f32   q13, q9, d8[1]      \n"
                        "vmla.f32   q14, q10, d5[0]     \n"
                        "vmla.f32   q15, q10, d9[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"
                        "vmla.f32   q13, q11, d9[1]     \n"

                        "pld        [%3, #512]          \n"
                        "vldm       %3!, {d0-d7}        \n"// r20 r21 r22 r23

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q14, q8, d0[0]      \n"
                        "vmla.f32   q15, q8, d4[0]      \n"
                        "vmla.f32   q12, q9, d0[1]      \n"
                        "vmla.f32   q13, q9, d4[1]      \n"
                        "vmla.f32   q14, q10, d1[0]     \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d1[1]     \n"
                        "vmla.f32   q13, q11, d5[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "pld        [%3, #128]          \n"
                        "vld1.f32   {d8-d9}, [%3 :128]  \n"// r24

                        "vmla.f32   q14, q8, d2[0]      \n"
                        "vmla.f32   q15, q8, d6[0]      \n"
                        "vmla.f32   q12, q9, d2[1]      \n"
                        "vmla.f32   q13, q9, d6[1]      \n"
                        "vmla.f32   q14, q10, d3[0]     \n"
                        "vmla.f32   q15, q10, d7[0]     \n"
                        "vmla.f32   q12, q11, d3[1]     \n"
                        "vmla.f32   q13, q11, d7[1]     \n"

//                         "pld        [%4, #512]          \n"
                        "vldm       %4, {d16-d23}       \n"

                        "vmla.f32   q14, q8, d4[0]      \n"
                        "vmla.f32   q15, q8, d8[0]      \n"
                        "vmla.f32   q12, q9, d4[1]      \n"
                        "vmla.f32   q13, q9, d8[1]      \n"
                        "vmla.f32   q14, q10, d5[0]     \n"
                        "vmla.f32   q15, q10, d9[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"
                        "vmla.f32   q13, q11, d9[1]     \n"

                        "vadd.f32   q12, q12, q14       \n"
                        "vadd.f32   q13, q13, q15       \n"

                        "sub        %4, %4, #512        \n"// kptr -= 8 * 16;

                        "vst1.f32   {d24-d27}, [%0 :128]! \n"

                        : "=r"(outptr0),    // %0
                          "=r"(r0),         // %1
                          "=r"(r1),         // %2
                          "=r"(r2),         // %3
                          "=r"(kptr)        // %4
                        : "0"(outptr0),
                          "1"(r0),
                          "2"(r1),
                          "3"(r2),
                          "4"(kptr)
                        : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
                    );
#endif // __aarch64__
                }
                for (; j<outw; j++)
                {
#if __aarch64__
                    asm volatile(
                        "prfm   pldl1keep, [%0, #128]       \n"
                        "ld1    {v20.4s}, [%0]              \n"// sum0

                        "prfm   pldl1keep, [%1, #384]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s}, [%1] \n"// r00 r01 r02

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmul   v21.4s, v16.4s, v0.s[0]     \n"
                        "fmul   v22.4s, v17.4s, v0.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmul   v23.4s, v18.4s, v0.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v0.s[3]     \n"

                        "fmla   v21.4s, v24.4s, v1.s[0]     \n"
                        "fmla   v22.4s, v25.4s, v1.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v23.4s, v26.4s, v1.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v1.s[3]     \n"

                        "prfm   pldl1keep, [%2, #384]       \n"
                        "ld1    {v3.4s, v4.4s, v5.4s}, [%2] \n"// r10 r11 r12

                        "fmla   v21.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v22.4s, v17.4s, v2.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v23.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v2.s[3]     \n"

                        "fmla   v21.4s, v24.4s, v3.s[0]     \n"
                        "fmla   v22.4s, v25.4s, v3.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v23.4s, v26.4s, v3.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v3.s[3]     \n"

                        "fmla   v21.4s, v16.4s, v4.s[0]     \n"
                        "fmla   v22.4s, v17.4s, v4.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v23.4s, v18.4s, v4.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v4.s[3]     \n"

                        "prfm   pldl1keep, [%3, #384]       \n"
                        "ld1    {v0.4s, v1.4s, v2.4s}, [%3] \n"// r20 r21 r22

                        "fmla   v21.4s, v24.4s, v5.s[0]     \n"
                        "fmla   v22.4s, v25.4s, v5.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4], #64 \n"

                        "fmla   v23.4s, v26.4s, v5.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v5.s[3]     \n"

                        "fmla   v21.4s, v16.4s, v0.s[0]     \n"
                        "fmla   v22.4s, v17.4s, v0.s[1]     \n"

                        "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v24.4s, v25.4s, v26.4s, v27.4s}, [%4], #64 \n"

                        "fmla   v23.4s, v18.4s, v0.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v0.s[3]     \n"

                        "fmla   v21.4s, v24.4s, v1.s[0]     \n"
                        "fmla   v22.4s, v25.4s, v1.s[1]     \n"

//                         "prfm   pldl1keep, [%4, #512]       \n"
                        "ld1    {v16.4s, v17.4s, v18.4s, v19.4s}, [%4] \n"

                        "fmla   v23.4s, v26.4s, v1.s[2]     \n"
                        "fmla   v20.4s, v27.4s, v1.s[3]     \n"

                        "fmla   v21.4s, v16.4s, v2.s[0]     \n"
                        "fmla   v22.4s, v17.4s, v2.s[1]     \n"
                        "fmla   v23.4s, v18.4s, v2.s[2]     \n"
                        "fmla   v20.4s, v19.4s, v2.s[3]     \n"

                        "add    %1, %1, #32                 \n"

                        "fadd   v22.4s, v21.4s, v22.4s      \n"

                        "add    %2, %2, #32                 \n"

                        "fadd   v23.4s, v23.4s, v22.4s      \n"

                        "add    %3, %3, #32                 \n"

                        "fadd   v20.4s, v20.4s, v23.4s      \n"

                        "sub    %4, %4, #512                \n"// kptr -= 8 * 16;

                        "st1    {v20.4s}, [%0], #16         \n"

                        : "=r"(outptr0),    // %0
                          "=r"(r0),         // %1
                          "=r"(r1),         // %2
                          "=r"(r2),         // %3
                          "=r"(kptr)        // %4
                        : "0"(outptr0),
                          "1"(r0),
                          "2"(r1),
                          "3"(r2),
                          "4"(kptr)
                        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27"
                    );
#else // __aarch64__
                    asm volatile(
                        "pld        [%0, #128]          \n"
                        "vld1.f32   {d24-d25}, [%0 :128] \n"// sum0

                        "pld        [%1, #384]          \n"
                        "vldm       %1, {d0-d5}         \n"// r00 r01 r02

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmul.f32   q13, q8, d0[0]      \n"
                        "vmul.f32   q14, q9, d0[1]      \n"
                        "vmul.f32   q15, q10, d1[0]     \n"
                        "vmla.f32   q12, q11, d1[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d2[0]      \n"
                        "vmla.f32   q14, q9, d2[1]      \n"
                        "vmla.f32   q15, q10, d3[0]     \n"
                        "vmla.f32   q12, q11, d3[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d4[0]      \n"
                        "vmla.f32   q14, q9, d4[1]      \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"

                        "pld        [%2, #384]          \n"
                        "vldm       %2, {d0-d5}         \n"// r10 r11 r12

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d0[0]      \n"
                        "vmla.f32   q14, q9, d0[1]      \n"
                        "vmla.f32   q15, q10, d1[0]     \n"
                        "vmla.f32   q12, q11, d1[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d2[0]      \n"
                        "vmla.f32   q14, q9, d2[1]      \n"
                        "vmla.f32   q15, q10, d3[0]     \n"
                        "vmla.f32   q12, q11, d3[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d4[0]      \n"
                        "vmla.f32   q14, q9, d4[1]      \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"

                        "pld        [%3, #384]          \n"
                        "vldm       %3, {d0-d5}         \n"// r20 r21 r22

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d0[0]      \n"
                        "vmla.f32   q14, q9, d0[1]      \n"
                        "vmla.f32   q15, q10, d1[0]     \n"
                        "vmla.f32   q12, q11, d1[1]     \n"

                        "pld        [%4, #512]          \n"
                        "vldm       %4!, {d16-d23}      \n"

                        "vmla.f32   q13, q8, d2[0]      \n"
                        "vmla.f32   q14, q9, d2[1]      \n"
                        "vmla.f32   q15, q10, d3[0]     \n"
                        "vmla.f32   q12, q11, d3[1]     \n"

//                         "pld        [%4, #512]          \n"
                        "vldm       %4, {d16-d23}       \n"

                        "vmla.f32   q13, q8, d4[0]      \n"
                        "vmla.f32   q14, q9, d4[1]      \n"
                        "vmla.f32   q15, q10, d5[0]     \n"
                        "vmla.f32   q12, q11, d5[1]     \n"

                        "vadd.f32   q14, q14, q13       \n"

                        "add        %1, %1, #32         \n"

                        "vadd.f32   q15, q15, q14       \n"

                        "add        %2, %2, #32         \n"

                        "vadd.f32   q12, q12, q15       \n"

                        "add        %3, %3, #32         \n"

                        "sub        %4, %4, #512        \n"// kptr -= 8 * 16;

                        "vst1.f32   {d24-d25}, [%0 :128]! \n"

                        : "=r"(outptr0),    // %0
                          "=r"(r0),         // %1
                          "=r"(r1),         // %2
                          "=r"(r2),         // %3
                          "=r"(kptr)        // %4
                        : "0"(outptr0),
                          "1"(r0),
                          "2"(r1),
                          "3"(r2),
                          "4"(kptr)
                        : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
                    );
#endif // __aarch64__
                }

                r0 += tailstep;
                r1 += tailstep;
                r2 += tailstep;
            }
        }
    }
}
