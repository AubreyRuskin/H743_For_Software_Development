/*	2013/6/5 读取SFP相关地址内的数据信息
*/
#include "vxWorks.h"

#include "stdio_compat.h"
#include "math_compat.h"
#include "bspinterface.h"

/* defines */

int i2cReadWithDevOffset
(
    FAST UINT i2cDevAdrs,           /* I2C device address */
    FAST UCHAR *pBuf,               /* buffer pointer (>= 16 bytes) */
    FAST UINT devOffset,            /* I2C address offset */
    FAST UINT offsetLen,            /* offset length*/
    FAST UINT byteCount,             /* I2C bytes to read */
    FAST UINT refer                      /* refer*/
)
{
    return 0;
};

int i2cWriteWithDevOffset
(
    FAST UINT i2cDevAdrs,           /* I2C device address */
    FAST UCHAR *pBuf,               /* buffer pointer (>= 16 bytes) */
    FAST UINT devOffset,            /* I2C address offset */
    FAST UINT offsetLen,            /* offset length*/
    FAST UINT byteCount,             /* I2C bytes to read */
    FAST UINT refer                      /* refer*/
);

int set_i2c_mux_val(unsigned char index)
{
    unsigned char buf[8];

    buf[0] = 1 << index;

    return i2cWriteWithDevOffset(I2C_MUX_ADRS, (UCHAR *)buf, 0, 0, 1, 0);
}

int	get_i2c_mux_val()
{
    int i;
    int ret_val = 0;
    unsigned char buf[8];

    ret_val = i2cReadWithDevOffset(I2C_MUX_ADRS, (UCHAR *)buf, 0, 0, 1, 0);

    if(0 == ret_val)
    {
        for(i=0; i<8; i++)
        {
            if(buf[0] & (1<<i))
            {
                break;
            }
        }

        if(8 == i)
        {
            ret_val = -1;
        }
    }

    return ret_val;
}

int get_sfp_status_val(unsigned char begin_adrs, unsigned char byte_cnt, unsigned char *buf)
{
    int ret_val = 0;

    buf[0] = begin_adrs;

    ret_val = i2cWriteWithDevOffset(SFP_STATUS_ADRS, (UCHAR *)buf, 0, 0, 1, 0);

    if(0 == ret_val)
    {
        ret_val = i2cReadWithDevOffset(SFP_STATUS_ADRS, (UCHAR *)buf, 0, 0, byte_cnt, 0);
    }

    return ret_val;
}

int get_sfp_data_val(unsigned char begin_adrs, unsigned char byte_cnt, unsigned char *buf)
{
    int ret_val = 0;

    buf[0] = begin_adrs;

    ret_val = i2cWriteWithDevOffset(SFP_DATA_ADRS, (UCHAR *)buf, 0, 0, 1, 0);

    if(0 == ret_val)
    {
        ret_val = i2cReadWithDevOffset(SFP_DATA_ADRS, (UCHAR *)buf, 0, 0, byte_cnt, 0);
    }

    return ret_val;
}

int	show_sfp_info(unsigned char index)
{
    int ret_val = 0;
    unsigned char buf[16];
    signed char temp_val;
    unsigned short vcc_ad_val;
    unsigned short tx_pwr_ad_val;
    unsigned short rx_pwr_ad_val;
    float vcc_val;
    float tx_pwr_val;
    float rx_pwr_val;

    ret_val = set_i2c_mux_val(index);

    if(0 == ret_val)
    {
        ret_val = get_sfp_status_val(SFP_TEMP_ADRS, 10, buf);
        if(0 == ret_val)
        {
            temp_val = (signed char)buf[0];
            vcc_ad_val = (buf[SFP_VCC_TO_TMP_OFFSET]<<8)
                         | buf[SFP_VCC_TO_TMP_OFFSET+1];
            tx_pwr_ad_val = (buf[SFP_TX_P_TO_TMP_OFFSET]<<8)
                            | buf[SFP_TX_P_TO_TMP_OFFSET+1];
            rx_pwr_ad_val = (buf[SFP_RX_P_TO_TMP_OFFSET]<<8)
                            | buf[SFP_RX_P_TO_TMP_OFFSET+1];

            vcc_val = VCC_SCALE * vcc_ad_val;
            tx_pwr_val = 30.0 + 10.0*log10(PWR_SCALE * tx_pwr_ad_val);
            rx_pwr_val = 30.0 + 10.0*log10(PWR_SCALE * rx_pwr_ad_val);

            printf("Temp is %d Centi, Vcc is %f V, Tx Pwr is %f dbm, Rx Pwr is %f dbm\n",
                   temp_val, vcc_val, tx_pwr_val, rx_pwr_val);
        }
    }

    if(ret_val != 0)
    {
        printf("Get SFP info fail.\n");
    }

    return ret_val;
}

int i2cWriteWithDevOffset
(
    FAST UINT i2cDevAdrs,           /* I2C device address */
    FAST UCHAR *pBuf,               /* buffer pointer (>= 16 bytes) */
    FAST UINT devOffset,            /* I2C address offset */
    FAST UINT offsetLen,            /* offset length*/
    FAST UINT byteCount,             /* I2C bytes to read */
    FAST UINT refer                      /* refer*/
){
    return 0;
}