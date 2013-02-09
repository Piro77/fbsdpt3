#define PT1_MAX_ISDB_S_INIT		19 // ISDB-S ������ǡ�����
#define PT1_MAX_ISDB_T_INIT		16 // ISDB-T ������ǡ�����
#define PT2_MAX_ISDB_S_INIT		18 // ISDB-S ������ǡ�����
#define PT2_MAX_ISDB_T_INIT		12 // ISDB-T ������ǡ�����

#define MAX_BS_CHANNEL			36 // ���ȿ��ơ��֥��
#define MAX_ISDB_T_CHANNEL		113 // ���ȿ��ơ��֥��(�ϥǥ�����)
#define MAX_BS_CHANNEL_PLL_COMMAND	3 // PLL��å����뤿��Υ��ޥ�ɿ�

#define MAX_BS_TS_ID			8 // TS-ID����������
#define MAX_ISDB_T_INFO			3 // �ϥǥ����ؾ����
#define MAX_ISDB_T_INFO_LEN		2 // �ϥǥ����ؾ����

typedef struct _WBLOCK_BS_PLL {
	const WBLOCK *wblock[MAX_BS_CHANNEL_PLL_COMMAND];
} WBLOCK_BS_PLL;

/***************************************************************************/
/* �����ϥơ��֥�                                                          */
/***************************************************************************/
/*
  ISDB-S�ξ���������
  C0 C1
  ����:7Bit Address Mode(1b/19):17:00
  ISDB-S�ξ�����̵��(�����ޥ��)
  C0 C1
  ����:7Bit Address Mode(1B/19):fe:c0:f0:04
  ����:7Bit Address Mode(1B/19):17:01
*/
static const WBLOCK isdb_s_wake = {
	0,
	4,
	{0xFE, 0xC0, 0xF0, 0x04}
};
static const WBLOCK isdb_s_sleep = {
	0,
	2,
	{0x17, 0x00}
};
/*
  ISDB-T�ξ���������
  C0 C1
  ����:7Bit Address Mode(1A/18):03:80

  ISDB-T�ξ�����̵��(�����ޥ��)
  C0 C1
  ����:7Bit Address Mode(1A/18):fe:c2
  ����:7Bit Address Mode(1A/18):03:90
*/

static const WBLOCK isdb_t_wake = {
	0,
	2,
	{0xFE, 0xC2}
};
static const WBLOCK isdb_t_sleep = {
	0,
	2,
	{0x03, 0x80}
};

/***************************************************************************/
/* ������ǡ������(����)                                                  */
/***************************************************************************/
static const WBLOCK com_initdata = {
	0,
	2,
	{0x01, 0x80}
};

/***************************************************************************/
/* ������ǡ������(ISDB-S)                                                */
/***************************************************************************/
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init1 ={
	0,
	1,
	{0x0f}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init2 ={
	0,
	2,
	{0x04, 0x02}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init3 ={
	0,
	2,
	{0x0D, 0x55} //pt1 only
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init4 ={
	0,
	2,
	{0x11, 0x40}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init5 ={
	0,
	2,
	{0x13, 0x80}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init6 ={
	0,
	2,
	{0x17, 0x01}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init7 ={
	0,
	2,
	{0x1C, 0x0A}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init8 ={
	0,
	2,
	{0x1D, 0xAA}
};
// ISDB-S������ͣ�
static const WBLOCK isdb_s_init9 ={
	0,
	2,
	{0x1E, 0x20}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init10 ={
	0,
	2,
	{0x1F, 0x88}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init11 ={
	0,
	2,
	{0x51, 0xB0}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init12 ={
	0,
	2,
	{0x52, 0x89}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init13 ={
	0,
	2,
	{0x53, 0xB3}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init14 ={
	0,
	2,
	{0x5A, 0x2D}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init15 ={
	0,
	2,
	{0x5B, 0xD3}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init16 ={
	0,
	2,
	{0x85, 0x69}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init17 ={
	0,
	2,
	{0x87, 0x04}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init18 ={
	0,
	2,
	{0x8E, 0x26}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init19 ={
	0,
	2,
	{0xA3, 0xF7}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init20 ={
	0,
	2,
	{0xA5, 0xC0}
};
// ISDB-S������ͣ���
static const WBLOCK isdb_s_init21 ={
	0,
	4,
	{0xFE, 0xC0, 0xF0, 0x04}
};
/***************************************************************************/
/* ������ǡ������(ISDB-T)                                                */
/***************************************************************************/
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init1 ={
	0,
	2,
	{0x03, 0x90}
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init2 ={
	0,
	2,
	{0x14, 0x8F} //pt1 only
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init3 ={
	0,
	2,
	{0x1C, 0x2A}
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init4 ={
	0,
	2,
	{0x1D, 0xA8}
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init5 ={
	0,
	2,
	{0x1E, 0xA2}
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init6 ={
	0,
	2,
	{0x22, 0x83}
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init7 ={
	0,
	2,
	{0x31, 0x0D} //pt1
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init8 ={
	0,
	2,
	{0x32, 0xE0} //pt1
};
// ISDB-T������ͣ�
static const WBLOCK isdb_t_init9 ={
	0,
	2,
	{0x39, 0xD3} //pt1
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init10 ={
	0,
	2,
	{0x3A, 0x00}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init11 ={
	0,
	2,
	{0x5C, 0x40}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init12 ={
	0,
	2,
	{0x5F, 0x80}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init13 ={
	0,
	2,
	{0x75, 0x0a}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init14 ={
	0,
	2,
	{0x76, 0x4c}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init15 ={
	0,
	2,
	{0x77, 0x03}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init16 ={
	0,
	2,
	{0xEF, 0x01}
};
// ISDB-T������ͣ���
static const WBLOCK isdb_t_init17 ={
	0,
	7,
	{0xFE, 0xC2, 0x01, 0x8F, 0xC1, 0x80, 0x80}
};
/***************************************************************************/
/* ������ǡ����֥�å����(ISDB-S)                                        */
/***************************************************************************/
static const WBLOCK *isdb_s_initial_pt1[PT1_MAX_ISDB_S_INIT] =
{
	&isdb_s_init2, &isdb_s_init3, &isdb_s_init4, &isdb_s_init5,
	&isdb_s_init6, &isdb_s_init7, &isdb_s_init8, &isdb_s_init9,
	&isdb_s_init10, &isdb_s_init11, &isdb_s_init12, &isdb_s_init13,
	&isdb_s_init14, &isdb_s_init15, &isdb_s_init16, &isdb_s_init17,
	&isdb_s_init18, &isdb_s_init19, &isdb_s_init20
};
static const WBLOCK *isdb_s_initial_pt2[PT2_MAX_ISDB_S_INIT] =
{
	&isdb_s_init2, &isdb_s_init4, &isdb_s_init5,
	&isdb_s_init6, &isdb_s_init7, &isdb_s_init8, &isdb_s_init9,
	&isdb_s_init10, &isdb_s_init11, &isdb_s_init12, &isdb_s_init13,
	&isdb_s_init14, &isdb_s_init15, &isdb_s_init16, &isdb_s_init17,
	&isdb_s_init18, &isdb_s_init19, &isdb_s_init20
};
/***************************************************************************/
/* ������ǡ����֥�å����(ISDB-T)                                        */
/***************************************************************************/
static const WBLOCK *isdb_t_initial_pt1[PT1_MAX_ISDB_T_INIT] =
{
	&isdb_t_init1, &isdb_t_init2, &isdb_t_init3, &isdb_t_init4,
	&isdb_t_init5, &isdb_t_init6, &isdb_t_init7, &isdb_t_init8,
	&isdb_t_init9, &isdb_t_init10, &isdb_t_init11, &isdb_t_init12,
	&isdb_t_init13, &isdb_t_init14, &isdb_t_init15, &isdb_t_init16
};
static const WBLOCK *isdb_t_initial_pt2[PT2_MAX_ISDB_T_INIT] =
{
	&isdb_t_init1, &isdb_t_init3, &isdb_t_init4,
	&isdb_t_init5, &isdb_t_init6,
	&isdb_t_init10, &isdb_t_init11, &isdb_t_init12,
	&isdb_t_init13, &isdb_t_init14, &isdb_t_init15, &isdb_t_init16
};
/***************************************************************************/
/* �Ͼ�ǥ������ѥǡ���                                                    */
/***************************************************************************/
/***************************************************************************/
/* ���ȿ�������ܥơ��֥�                                                  */
/* 0��1: ����                                                              */
/* 2��3: �׻����                                                          */
/* 4��5: �ɲ÷׻����                                                      */
/***************************************************************************/

static const WBLOCK isdb_t_pll_base = {
	0,
	2,
	{0xFE, 0xC2, 0, 0, 0, 0, 0, 0}
};
/***************************************************************************/
/* �ϥǥ����ȿ���å������å�                                              */
/***************************************************************************/
static const WBLOCK isdb_t_pll_lock = {
	0,
	2,
	{0xFE, 0xC3}
};

static const WBLOCK isdb_t_check_tune = {
	0,
	2,
	{0x01, 0x40}
};

static const WBLOCK isdb_t_tune_read = {
	0,
	1,
	{0x80}
};
static const WBLOCK isdb_t_tmcc_read_1 = {
	0,
	1,
	{0xB2}
};
static const WBLOCK isdb_t_tmcc_read_2 = {
	0,
	1,
	{0xB6}
};
/***************************************************************************/
/* �ϥǥ����ȿ���å������å�                                              */
/***************************************************************************/
static const WBLOCK isdb_t_signal1 = {
	0,
	1,
	{0x8C}
};
static const WBLOCK isdb_t_signal2 = {
	0,
	1,
	{0x8D}
};
static const WBLOCK isdb_t_agc2 = {
	0,
	1,
	{0x82}
};
static const WBLOCK isdb_t_lockedt1 = {
	0,
	1,
	{0x96}
};
static const WBLOCK isdb_t_lockedt2 = {
	0,
	1,
	{0xB0}
};
static const WBLOCK isdb_t_get_clock = {
	0,
	1,
	{0x86}
};
static const WBLOCK isdb_t_get_carrir = {
	0,
	1,
	{0x84}
};

/***************************************************************************/
/* �ϥǥ��ѥǡ���                                                          */
/***************************************************************************/

/***************************************************************************/
/* �£��ѥǡ���                                                            */
/***************************************************************************/
/***************************************************************************/
/* �£Ӽ��ȿ���å������å�                                                */
/***************************************************************************/
static const WBLOCK bs_pll_lock = {
	0,
	2,
	{0xFE, 0xC1}
};
/***************************************************************************/
/* TMCC����                                                                */
/***************************************************************************/
static const WBLOCK bs_tmcc_get_1 = {
	0,
	2,
	{0x03, 0x01}
};
static const WBLOCK bs_tmcc_get_2 = {
	0,
	1,
	{0xC3}
};
/***************************************************************************/
/* TMCC����                                                                */
/***************************************************************************/
static const WBLOCK bs_get_slot_ts_id_1 = {
	0,
	1,
	{0xCE}
};
static const WBLOCK bs_get_slot_ts_id_2 = {
	0,
	1,
	{0xD2}
};
static const WBLOCK bs_get_slot_ts_id_3 = {
	0,
	1,
	{0xD6}
};
static const WBLOCK bs_get_slot_ts_id_4 = {
	0,
	1,
	{0xDA}
};
/***************************************************************************/
/* TS-ID��å�                                                             */
/***************************************************************************/
static const WBLOCK bs_set_ts_lock = {
	0,
	3,
	{0x8F, 0x00, 0x00}
};
/***************************************************************************/
/* TS-ID����                                                               */
/***************************************************************************/
static const WBLOCK bs_get_ts_lock = {
	0,
	1,
	{0xE6}
};
/***************************************************************************/
/* ����åȼ���                                                            */
/***************************************************************************/
static const WBLOCK bs_get_slot = {
	0,
	1,
	{0xE8}
};
/***************************************************************************/
/* CN/AGC/MAXAGC����                                                       */
/***************************************************************************/
static const WBLOCK bs_get_signal1 = {
	0,
	1,
	{0xBC}
};
static const WBLOCK bs_get_signal2 = {
	0,
	1,
	{0xBD}
};
static const WBLOCK bs_get_agc = {
	0,
	1,
	{0xBA}
};
/***************************************************************************/
/* ����å����ȿ�������                                                  */
/***************************************************************************/
static const WBLOCK bs_get_clock = {
	0,
	1,
	{0xBE}
};
/***************************************************************************/
/* ����ꥢ���ȿ�������                                                  */
/***************************************************************************/
static const WBLOCK bs_get_carrir = {
	0,
	1,
	{0xBB}
};
/***************************************************************************/
/* ���ȿ�����ơ��֥�                                                      */
/* �£Ӥ˴ؤ��ƤΤߡ��Ȥꤢ�����ơ��֥�Ȥ��������׻��ǻ��н����ʤ�      */
/* �׻��ǻ��Ф����롣                                                      */
/***************************************************************************/
/***************************************************************************/
/* BS���̥ơ��֥�                                                          */
/***************************************************************************/
static const WBLOCK bs_com_step2 = {
	0,
	3,
	{0xFE, 0xC0, 0xE4}
};
/***************************************************************************/
/* BS-1                                                                    */
/***************************************************************************/
static const WBLOCK bs_1_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x48, 0x29, 0xE0, 0xD2}
};
static const WBLOCK bs_1_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xD6}
};
/***************************************************************************/
/* BS-3                                                                    */
/***************************************************************************/
static const WBLOCK bs_3_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x44, 0x40, 0xE0, 0xE2}
};
static const WBLOCK bs_3_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xE6}
};
/***************************************************************************/
/* BS-5                                                                    */
/***************************************************************************/
static const WBLOCK bs_5_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x44, 0x66, 0xE0, 0xE2}
};
static const WBLOCK bs_5_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xE6}
};
/***************************************************************************/
/* BS-7                                                                    */
/***************************************************************************/
static const WBLOCK bs_7_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x44, 0x8D, 0xE0, 0x20}
};
static const WBLOCK bs_7_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x24}
};
/***************************************************************************/
/* BS-9                                                                    */
/***************************************************************************/
static const WBLOCK bs_9_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x44, 0xB3, 0xE0, 0x20}
};
static const WBLOCK bs_9_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x24}
};
/***************************************************************************/
/* BS-11                                                                   */
/***************************************************************************/
static const WBLOCK bs_11_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x44, 0xD9, 0xE0, 0x20}
};
static const WBLOCK bs_11_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x24}
};
/***************************************************************************/
/* BS-13                                                                   */
/***************************************************************************/
static const WBLOCK bs_13_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x45, 0x00, 0xE0, 0x20}
};
static const WBLOCK bs_13_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x24}
};
/***************************************************************************/
/* BS-15                                                                   */
/***************************************************************************/
static const WBLOCK bs_15_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x45, 0x26, 0xE0, 0x40}
};
static const WBLOCK bs_15_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x44}
};
/***************************************************************************/
/* BS-17                                                                   */
/***************************************************************************/
static const WBLOCK bs_17_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x45, 0x4C, 0xE0, 0x40}
};
static const WBLOCK bs_17_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0X44}
};
/***************************************************************************/
/* BS-19                                                                   */
/***************************************************************************/
static const WBLOCK bs_19_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x45, 0x73, 0xE0, 0x40}
};
static const WBLOCK bs_19_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x44}
};
/***************************************************************************/
/* BS-21                                                                   */
/***************************************************************************/
static const WBLOCK bs_21_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x45, 0x99, 0xE0, 0x40}
};
static const WBLOCK bs_21_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x44}
};
/***************************************************************************/
/* BS-23                                                                   */
/***************************************************************************/
static const WBLOCK bs_23_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x45, 0xBF, 0xE0, 0x60}
};
static const WBLOCK bs_23_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x64}
};

/***************************************************************************/
/* ND 2                                                                    */
/***************************************************************************/
static const WBLOCK nd_2_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0x4D, 0xE0, 0x60}
};
static const WBLOCK nd_2_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x64}
};

/***************************************************************************/
/* ND 4                                                                    */
/***************************************************************************/
static const WBLOCK nd_4_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0x75, 0xE0, 0x80}
};
static const WBLOCK nd_4_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 6                                                                    */
/***************************************************************************/
static const WBLOCK nd_6_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0x9D, 0xE0, 0x80}
};
static const WBLOCK nd_6_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 8                                                                    */
/***************************************************************************/
static const WBLOCK nd_8_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0xC5, 0xE0, 0x80}
};
static const WBLOCK nd_8_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 10                                                                   */
/***************************************************************************/
static const WBLOCK nd_10_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0xED, 0xE0, 0x80}
};
static const WBLOCK nd_10_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 12                                                                   */
/***************************************************************************/
static const WBLOCK nd_12_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x15, 0xE0, 0xA0}
};
static const WBLOCK nd_12_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 14                                                                   */
/***************************************************************************/
static const WBLOCK nd_14_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x3D, 0xE0, 0xA0}
};
static const WBLOCK nd_14_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 16                                                                   */
/***************************************************************************/
static const WBLOCK nd_16_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x65, 0xE0, 0xA0}
};
static const WBLOCK nd_16_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 18                                                                   */
/***************************************************************************/
static const WBLOCK nd_18_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x8D, 0xE0, 0xA0}
};
static const WBLOCK nd_18_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 20                                                                   */
/***************************************************************************/
static const WBLOCK nd_20_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0xB5, 0xE0, 0xC0}
};
static const WBLOCK nd_20_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xC4}
};

/***************************************************************************/
/* ND 22                                                                   */
/***************************************************************************/
static const WBLOCK nd_22_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0xDD, 0xE0, 0xC0}
};
static const WBLOCK nd_22_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xC4}
};

/***************************************************************************/
/* ND 24                                                                   */
/***************************************************************************/
static const WBLOCK nd_24_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x48, 0x05, 0xE0, 0xC0}
};
static const WBLOCK nd_24_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xC4}
};

/***************************************************************************/
/* ND 1                                                                    */
/***************************************************************************/
static const WBLOCK nd_1_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0x39, 0xE0, 0x60}
};
static const WBLOCK nd_1_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x64}
};

/***************************************************************************/
/* ND 3                                                                    */
/***************************************************************************/
static const WBLOCK nd_3_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0x61, 0xE0, 0x80}
};
static const WBLOCK nd_3_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 5                                                                    */
/***************************************************************************/
static const WBLOCK nd_5_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0x89, 0xE0, 0x80}
};
static const WBLOCK nd_5_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 7                                                                    */
/***************************************************************************/
static const WBLOCK nd_7_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0xB1, 0xE0, 0x80}
};
static const WBLOCK nd_7_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 9                                                                    */
/***************************************************************************/
static const WBLOCK nd_9_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x46, 0xD9, 0xE0, 0x80}
};
static const WBLOCK nd_9_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0x84}
};

/***************************************************************************/
/* ND 11                                                                   */
/***************************************************************************/
static const WBLOCK nd_11_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x01, 0xE0, 0xA0}
};
static const WBLOCK nd_11_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 13                                                                   */
/***************************************************************************/
static const WBLOCK nd_13_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x29, 0xE0, 0xA0}
};
static const WBLOCK nd_13_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 15                                                                   */
/***************************************************************************/
static const WBLOCK nd_15_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x51, 0xE0, 0xA0}
};
static const WBLOCK nd_15_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 17                                                                   */
/***************************************************************************/
static const WBLOCK nd_17_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0x79, 0xE0, 0xA0}
};
static const WBLOCK nd_17_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 19                                                                   */
/***************************************************************************/
static const WBLOCK nd_19_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0xA1, 0xE0, 0xA0}
};
static const WBLOCK nd_19_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xA4}
};

/***************************************************************************/
/* ND 21                                                                   */
/***************************************************************************/
static const WBLOCK nd_21_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0xC9, 0xE0, 0xC0}
};
static const WBLOCK nd_21_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xC4}
};

/***************************************************************************/
/* ND 23                                                                   */
/***************************************************************************/
static const WBLOCK nd_23_step1 = {
	0,
	6,
	{0xFE, 0xC0, 0x47, 0xF1, 0xE0, 0xC0}
};
static const WBLOCK nd_23_step3 = {
	0,
	4,
	{0xFE, 0xC0, 0xF4, 0xC4}
};

/***************************************************************************/
/* BS-���ȿ��ơ��֥�                                                       */
/***************************************************************************/
static const WBLOCK_BS_PLL bs_pll[MAX_BS_CHANNEL] = {
	{{&bs_1_step1, &bs_com_step2, &bs_1_step3}},
	{{&bs_3_step1, &bs_com_step2, &bs_3_step3}},
	{{&bs_5_step1, &bs_com_step2, &bs_5_step3}},
	{{&bs_7_step1, &bs_com_step2, &bs_7_step3}},
	{{&bs_9_step1, &bs_com_step2, &bs_9_step3}},
	{{&bs_11_step1, &bs_com_step2, &bs_11_step3}},
	{{&bs_13_step1, &bs_com_step2, &bs_13_step3}},
	{{&bs_15_step1, &bs_com_step2, &bs_15_step3}},
	{{&bs_17_step1, &bs_com_step2, &bs_17_step3}},
	{{&bs_19_step1, &bs_com_step2, &bs_19_step3}},
	{{&bs_21_step1, &bs_com_step2, &bs_21_step3}},
	{{&bs_23_step1, &bs_com_step2, &bs_21_step3}},
	{{&nd_2_step1, &bs_com_step2, &nd_2_step3}},
	{{&nd_4_step1, &bs_com_step2, &nd_4_step3}},
	{{&nd_6_step1, &bs_com_step2, &nd_6_step3}},
	{{&nd_8_step1, &bs_com_step2, &nd_8_step3}},
	{{&nd_10_step1, &bs_com_step2, &nd_10_step3}},
	{{&nd_12_step1, &bs_com_step2, &nd_12_step3}},
	{{&nd_14_step1, &bs_com_step2, &nd_14_step3}},
	{{&nd_16_step1, &bs_com_step2, &nd_16_step3}},
	{{&nd_18_step1, &bs_com_step2, &nd_18_step3}},
	{{&nd_20_step1, &bs_com_step2, &nd_20_step3}},
	{{&nd_22_step1, &bs_com_step2, &nd_22_step3}},
	{{&nd_24_step1, &bs_com_step2, &nd_24_step3}},
	{{&nd_1_step1, &bs_com_step2, &nd_1_step3}},
	{{&nd_3_step1, &bs_com_step2, &nd_3_step3}},
	{{&nd_5_step1, &bs_com_step2, &nd_5_step3}},
	{{&nd_7_step1, &bs_com_step2, &nd_7_step3}},
	{{&nd_9_step1, &bs_com_step2, &nd_9_step3}},
	{{&nd_11_step1, &bs_com_step2, &nd_11_step3}},
	{{&nd_13_step1, &bs_com_step2, &nd_13_step3}},
	{{&nd_15_step1, &bs_com_step2, &nd_15_step3}},
	{{&nd_17_step1, &bs_com_step2, &nd_17_step3}},
	{{&nd_19_step1, &bs_com_step2, &nd_19_step3}},
	{{&nd_21_step1, &bs_com_step2, &nd_21_step3}},
	{{&nd_23_step1, &bs_com_step2, &nd_23_step3}}
};
static const WBLOCK *bs_get_ts_id[(MAX_BS_TS_ID / 2)] = {
	&bs_get_slot_ts_id_1,
	&bs_get_slot_ts_id_2,
	&bs_get_slot_ts_id_3,
	&bs_get_slot_ts_id_4
};
