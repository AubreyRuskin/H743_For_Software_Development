#ifndef COM_FUNC_H
#define COM_FUNC_H
#include "edpbase.h"
#include "miscfunc.h"
#include "com_api.h"
#include "datetime.h"
extern "C"
{
    /*通讯初始化函数
    *参数：		无
    *返回值：	EP_SUCCESS，初始化成功
    *			EP_ERROR，初始化失败*/
    EP_STATUS	com_init();

    /*遥控
    *参数：	   	ucType,		遥控类型
    *			ucObject,	遥控对象
    *			ucCode,		遥控操作代码
    *返回值：  	EP_SUCCESS，正常返回
    *          	EP_ERROR，  错误返回 */
    EP_STATUS	RemoteCtrl(uint8_t ucType,uint8_t ucObject,uint8_t ucCode);

    /*取得遥测量
    *参数：		pRMV，保存遥测量量数据的内存地址
    *			RMV_Num,	要读取的遥测量个数
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，读取失败*/
    EP_STATUS	Get_RMV(uint16_t RMV_Num,RMV * pRMV);

    /*读取开入量状态
    *参数：		pKRstatus，	指向保存开入量状态数据的地址
    *			KR_Num,		要读取的开入量个数
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，  读取失败*/

    EP_STATUS 	GetKRstatus(uint8_t KR_Num,KRstatus * pKRstatus);

    /*取得遥信量
    *参数：		pRSV，保存遥测量量数据的内存地址
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，读取失败*/
    EP_STATUS	Get_RSV(RSV * pRSV);

    /*取得一个遥测量
    *参数：		pRMV，保存遥测量量数据的内存地址
    *			SN,	要读取的遥测量的序号
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，读取失败*/
    EP_STATUS	Get_ONE_RMV(uint16_t SN,RMV * pRMV);

    /*取得一个遥信量
    *参数：		SN，要取得的遥信量的序号
    *			pRSV，保存遥测量量数据的内存地址
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，读取失败*/
    EP_STATUS	Get_ONE_RSV(uint8_t SN,RSV * pRSV);

    /*传送数据到CPU的文件中
     *参数：	pcFileName,	文件名
     *参数：	pucData,	数据地址
     *参数：	lLength,	数据长度
     *返回值：  >=0，实际传送字节数
     *          EP_ERROR，  错误返回 */
    EP_STATUS	WriteFile(char *pcFileName,uint8_t *pucData,int32_t lLength);

    /*从CPU的文件中读取数据
     *参数：	pcFileName,	文件名
     *参数：	pucData,	数据地址
     *参数：	lMaxLength,	最大数据长度
     *返回值：  >=0，实际传送字节数
     *          EP_ERROR，  错误返回 */
    EP_STATUS	ReadFile(char *pcFileName,uint8_t *pucData,int32_t lMaxLength);

    /*读取日历时钟
    * 参数:		pdttmNow, 保存日期时间
    * 返回值:	EP_SUCCESS, 读取成功
    *			EP_ERROR,读取失败*/

    EP_STATUS 	Get_Sys_Time(EP_DATE_TIME *pdttmNow);

    /*设置日历时钟
    * 参数:		pdttmSet, 要设置的日期时间
    * 返回值:	EP_SUCCESS, 读取成功
    *			EP_ERROR,读取失败*/

    EP_STATUS 	Set_Sys_Time(EP_DATE_TIME *pdttmSet,EP_DATE_TIME *pdttmNow);

    /*读取所有有效定值区号
    *参数：		pValidSetAreaNum，保存有效定值区号数据的内存地址
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，  读取失败*/

//EP_STATUS	GetValidSetAreaNum(ValidSetAreaNum * pValidSetAreaNum);

    /*读取运行定值区号
    *参数：		pRunSetAreaNum，保存有效定值区号数据的内存地址
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，  读取失败*/

    EP_STATUS	GetRunSetAreaNum(uint8_t * pRunSetAreaNum);

    /*固化定值区文件
    *参数：		SN，待固化的定值区号
    *			pFileName,该定值区文件的文件名
    *返回值：	EP_SUCCESS，读取成功
    *			EP_ERROR，  读取失败*/

    EP_STATUS	SolidifySetAreaFile(uint8_t SN, uint8_t * FileName);
}

#endif

