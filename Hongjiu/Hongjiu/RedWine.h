#ifndef _INCLUDE_INFOR_REDWINE_H
#define _INCLUDE_INFOR_REDWINE_H


#include <string>
#include <vector>
#include <map>
#include <algorithm>

class CBlockTcp;
struct ZXCMDHEAD;
class CWriteBuff;
struct RedWineMark;
struct MonthStatmentTag;
struct ResponseDataBuf;
struct RedWineNameS;
struct RedwineCapacityS;
struct RedwineParkRatinS;
struct RedwineBaseInforTag;
struct RedwineAuctionResMark;

typedef RedWineMark* (*FindTypeMemMark)(const int index);				// 查找标识函数指针函数

/// 因为酒类指数历史数据可能不连续取，因此数据中已经包含表头
/// 可以一次请求的数据部分也包含表头
/// 分次请求并且数据连续的请求的时候添加表头

class CRedWine
{
public:
	CRedWine(void);
	virtual ~CRedWine(void);

// private variable
private:
	// 网络服务对象
	static CBlockTcp *m_pSocketServer;
	// 是否已连接
	static bool m_bConnected;
	// 包序列号
	static int m_PackSerialNum;
	// 当前包序号
	static int m_LastestPackSerialNum;
	// 当前获取数据失败的返回值
	static int m_iLastestErrorCode;
	
// private function
private:
	// 字符串转换成unsigned long
	static unsigned long StrToUl(const std::string & data);
	// 字符串转换成doulbe
	static double StrToDouble(const std::string & data);
	// 字符串拷贝
	static bool StrNCpy(char *buf, const int StartPos, const int MaxSize, const std::string & data);
	// 转换成int
	static int StrToInt(const std::string & data);
	// 是否为空串
	static bool IsEmptyString(const char* data);
	// 设置货币单位到属性字段的最高位
	static bool SetHByteMoneyUnitToProperty(const unsigned char MoneyUnitValue, unsigned long &Property);
	// 设置酒种类索引到属性字段的除最高位外的低位
	static bool SetExHByteSerialToProperty(const unsigned long MoneyUnitValue, unsigned long &Property);
	// 发送请求数据
	static bool SendRequestData(void *ReqData);
	// 读取响应请求
	static int RecvResponseData();
	// 获取发送序列号
	static int GetSendSeqNum(const int type = 0);
	// 获取最新包序列
	static int GetLastestSeqNum();
	// 数据区域初始化
	static void InitialDatacache();
	// 注册取数据函数
	static void RegisterRequestFun();
	// 执行取数据函数
	static void DoRequestStaticFunList();

	// 获取特定的数据索引
	// 类型关键字定义: 
	//	'WineIndexList'					指数列表 
	//	'WineIndexHistory'				指数历史数据
	//	'RedWineCritic'					酒评家及机构数据
	//	'RedWineAreaClassified'			产区分类数据
	//	'RedWineParentArea'				红酒大产区数据
	//	'RedWineChildArea'				红酒子产区数据
	//	'EightWineries'					八大酒庄列表
	//	'LivLastestPriceList'			Liv最新报价列表
	//	'MemListIndex'					列表指数成分股索引数据
	//	'RedWineSaleArea'				销售区域数据
	//	'WineLivexDayPrice'				Liv价格周期为日的数据
	//	'WineAuctionDayPrice'			拍卖价格周期为日的数据
	//	'LivMonthPriceHisData'			按月统计Liv数据
	//	'AuctionMonthPriceHisData'		按月统计拍卖数据
	//	'RetailMonthPriceHisData'		按月统计零售数据
	//	'RedwineMoneyUnit'				货币单位数据
	//	'RedWineSaleAddress'			红酒销售地点数据
	//	'RedWineAuctionHouse'			红酒拍卖行数据
	//	'RedWineAuctionSchedule'		红酒拍卖会日程数据
	//	'RedWineCapacityList'			红酒容量
	//	'RedWineNameList'				红酒名称
	//	'RedWineBaseInforList'			红酒基本信息
	//	'RedWineLivMemList'				Liv市场的全部成分酒信息
	//	'RedWineAutionMemList'			拍卖市场的全部成分酒信息
	//	'RedWineRetailMemList'			零售市场全部成分酒
	//	'RedwineRetailLatestPrice'		零售最新报价
	//	'RedWineClassifiedNameListIndex'红酒按照市场分类的名称列表列表索引
	//	'AuctionResSearchMemEnum'		拍卖结果查询酒标识列表
	//	'AuctionResSearchKeyList'		拍卖结果查询关键字
	//	'AuctionResSearchDataList'		拍卖结果查询全数据
	//	'AuctionResByWineTypeDataList'	拍卖按照红酒种类归类查询全数据
	static const ResponseDataBuf* GetTypeWineData(const std::string &type);

	// 请求酒类指数列表
	static bool RequestWineIndexList_S();
	// 解析红酒指数列表
	static bool ParseWineIndexList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	
	// 请求酒类指数全数据
	static bool GetWineIndexData_S();
	// 请求酒类指数分类数据
	static bool RequestWineIndexClassified(const int TotalCount, const int Serial, const char* KeyWords,
						const int IndexPeriod, CWriteBuff *bufWrite);	
	// 解析红酒指数历史数据
	static bool ParseWineIndexClassified(const int TotalCount, const int Serial, const char* KeyWords, 
						void *ReqData, char *data, const int length, CWriteBuff *bufWrite);

	// 请求酒评家及机构数据
	static bool RequestRedWineCritic_S();
	// 解析酒评家及机构数据
	static bool ParseRedWineCritic(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定的酒评家
	static int FindCritic(const std::string &Key);

	// 请求红酒产区分类数据
	static bool RequestWineAreaClassified_S();
	// 查找产区分类索引数据
	static int FindAreaClassified(const std::string &Key);

	// 请求红酒大产区数据
	static bool RequestRedWineParentArea_S();
	// 解析红酒大产区数据
	static bool ParseRedWineParentArea(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定父产区的列表索引
	static int FindParentArea(const std::string &Key);

	// 请求红酒子产区数据
	static bool RequestRedWineChildArea_S();
	// 解析红酒子产区数据
	static bool ParseRedWineChildArea(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定子产区的列表索引
	static int FindChildArea(const std::string &Key);
	
	// 生成八大酒庄信息
	static bool GetEightWineriesData_S();

	// 请求Liv报价最新列表
	static bool RequestLivLatestPriceList_S();
	// 解析Liv报价最新列表
	static bool ParseLivLatestPriceList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	
	// 请求销售区域数据
	static bool RequestSaleArea_S();
	// 查找销售区域
	static int FindSaleArea(const std::string &Key);
	
	
	// 请求单只指数成分股在List中的索引
	static bool RequestSingleIndexMemPosList_S(const int TotalCount, const int Serial, 
						const char* KeyWords, CWriteBuff *bufWrite);
	// 解析单只指数成分股在List中的索引
	static bool ParseSingleIndexMemPosList_S(const int TotalCount, const int Serial, const char* KeyWords, 
						void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 请求全部指数成分股索引数据
	static bool RequestAllIndexMemPosList_S();

	
	// 获取Liv-Ex价格日周期全部数据
	static bool RequestLivexDayPrice_S();
	// 解析Liv-Ex价格日周期全部数据
	static bool ParseLivexDayPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// 拍卖价格日周期全部数据源
	static bool RequestWineAuctionDayPrice_S();
	// 解析拍卖日周期数据
	static bool ParseWineAuctionDayPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// 请求月统计数据
	static bool RequestMonthPrice(const MonthStatmentTag *MarkConditon);
	// 解析月统计数据
	static bool ParseMonthPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   const int CmdType, FindTypeMemMark markFun, CWriteBuff *bufWrite, size_t &ItemCount);
	// 解析Liv月统计数据
	static bool ParseLivMonthPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   const int CmdType, FindTypeMemMark markFun, CWriteBuff *bufWrite, size_t &ItemCount);
	// 请求全部月统计数据
	static bool RequestMonthPrice_S();

	// 请求货币单位数据
	static bool ReqeustMoneyUnit_S();
	// 查找货币单位
	static int FindMoneyUnit(const std::string &Key);

	// 请求红酒销售地点信息
	static bool RequestSaleAddress_S();
	// 解析红酒销售地点信息
	static bool ParseSaleAddress(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找销售地点索引
	static int FindSaleAddress(const std::string &Key);
	// 获取指定的销售地点所属的区域
	static int GetSaleAddressBelongArea(const std::string &Key);

	// 请求红酒拍卖行数据
	static bool RequestAuctionHouse_S();
	// 解析红酒拍卖行数据
	static bool ParseAuctionHouse(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找拍卖行索引
	static int FindAuctionHouse(const std::string &Key);

	// 请求红酒拍卖会日程数据
	static bool RequestAuctionSchedule_S();
	// 解析红酒拍卖会日程数据
	static bool ParseAuctionSchedule(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);

	// 请求红酒名称列表数据
	static bool RequestRedwineNameList_S();
	// 解析红酒名称列表数据
	static bool ParseRedwineNameList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找红酒名称
	static int FindRedwineName(const std::string &Key); // 酒名称代码 作为关键字
	// 获取指定索引位置的名称
	static const RedWineNameS* GetIndexRedWineName(const int index);

	// 请求红酒容量列表数据
	static bool RequestRedwineCapacityList_S();
	// 解析红酒容量列表数据
	static bool ParseRedwineCapacityList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找红酒容量
	static int FindRedwineCapacity(const std::string &Key);	// 容量值 作为关键字
	// 获取指定索引位置的容量
	static const RedwineCapacityS* GetIndexRedWineCapacity(const int index);

	// 请求帕克分数
	static bool RequestRedwineParkRating();
	// 解析帕克分数
	static bool ParseRedwineParkRating(void *ReqData, char *data, const int length);
	// 查找帕克分数
	static const RedwineParkRatinS* FindRedwineParkRating(const std::string &Key); // 酒名称代码+年份 作为关键字

	// 请求红酒基本信息
	static bool RequestRedwineBaseInfor_S();
	// 解析红酒基本信息
	static bool ParseRedwineBaseInfor(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找红酒基本信息
	static int FindRedwineBaseInfor(const std::string &Key); // 酒名称代码 作为关键字
	// 获取指定索引的红酒基本信息
	static const RedwineBaseInforTag* GetIndexBaseInfor(const int index);

	// 请求Liv市场全部成分酒数据
	static bool RequestLivMemberList_S();
	// 解析Liv市场全部成分酒数据
	static bool ParseLivMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定酒标识的索引
	static int FindLivMemMarkIndex(const std::string &Key);  // 酒名称代码+年份+容量+规格 作为关键字
	// 查找指定酒的标识ID
	static RedWineMark* FindLivMemMark(const int index);
	// 请求Liv市场成员的日周期历史数据数目
	static bool RequestLivMemberDayPriceElement();
	// 解析Liv市场成员的日周期历史数据数目
	static bool ParseLivMemberDayPriceElement(void *ReqData, char *data, const int length);
	// 请求Liv市场成员的月周期历史数据数目
	static bool RequestLivMemberMonthPriceElement();
	// 解析Liv市场成员的月周期历史数据数目
	static bool ParseLivMemberMonthPriceElement(void *ReqData, char *data, const int length);

	// 请求拍卖市场全部成分酒数据
	static bool RequestAutionMemberList_S();
	// 解析拍卖市场全部成分酒数据
	static bool ParseAuctionMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定酒标识的索引
	static int FindAuctionMemMarkIndex(const std::string &Key);  // 酒名称代码+年份+容量 作为关键字
	// 查找指定酒的标识ID
	static RedWineMark* FindAuctionMemMark(const int index);
	// 请求拍卖市场成员的日周期历史数据数目
	static bool RequestAuctionMemberDayPriceElement();
	// 解析拍卖市场成员的日周期历史数据数目
	static bool ParseAuctionMemberDayPriceElement(void *ReqData, char *data, const int length);
	// 请求拍卖市场成员的月周期历史数据数目
	static bool RequestAuctionMemberMonthPriceElement();
	// 解析拍卖市场成员的月周期历史数据数目
	static bool ParseAuctionMemberMonthPriceElement(void *ReqData, char *data, const int length);
	
	// 请求零售市场全部成分酒数据(已经包含统计月统计信息)
	static bool RequestRetailMemberList_S();
	// 解析零售市场全部成分酒数据
	static bool ParseRetailMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定酒标识的索引
	static int FindRetailMemMarkIndex(const std::string &Key);  // 酒名称代码+年份+容量+数量 作为关键字
	// 查找指定酒的标识ID
	static RedWineMark* FindRetailMemMark(const int index);
	// 请求零售最新报价
	static bool RequestRetailLatestPrice_S();
	// 解析零售最新报价
	static bool ParseRetailLatestPrice(int &Serial, void *ReqData, char *data, const int length, CWriteBuff *bufWrite, size_t &ItemCount);

	// 请求红酒三个市场酒名称索引列表
	static bool RequestClassifiedNameListIndex_S();
	// 解析红酒三个市场酒名称索引列表
	static bool ParseClassifiedNameListIndex(const int TotalCount, int &Serial, void *ReqData, char *data, 
		const int length, CWriteBuff *bufWrite);

	// 请求红酒拍卖结果查询结果数据酒标识列表
	static bool RequestAuctionResSearchMemEnum_S();
	// 解析红酒拍卖结果查询结果数据酒标识列表
	static bool ParseAuctionResSearchMemEnum(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找指定酒标识的索引
	static int FindAuctionResSearchMemMarkIndex(const std::string &Key);  // 酒名称代码+年份+容量 作为关键字
	// 返回拍卖指定酒标识
	static RedWineMark* FindAuctionResSearchMemMark(const int index);

	// 请求红酒拍卖结果查询关键字 (包含数量统计)
	static bool RequestAuctionResSearchKey_S();
	// 解析红酒拍卖结果查询关键字(地点+拍卖机构)组合
	static bool ParseAuctionResSearchKey(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// 查找拍卖结果查询关键字的索引
	static int FindAuctionResSearchKeyIndex(const std::string &Key);  // 拍卖地点+拍卖行 作为关键字
	// 查找拍卖结果查询关键字ID
	static RedwineAuctionResMark* FindAuctionResSearchKey(const int index);

	// 获取拍卖结果全部数据
	static bool RequestAuctionResSearchData_S();
	// 解析拍卖结果全部数据
	static bool ParseAuctionResSearchData(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// 请求拍卖结果按照酒种类划分的全部数据
	static bool RequestAuctionResSingleTypeData_S();
	// 解析拍卖结果按照酒种类划分的全部数据
	static bool ParseAuctionResSingleTypeData(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// 当请求的数据不存在的时候，插入空包头
	static bool AddEmptyHeadData(const int TotalCount, const int Serial, const int HeadCmdType, CWriteBuff *bufWrite);

private:
	// 一次请求数据处理
	const int ProcGetTotalTypeData(const std::string &DataType, const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 分次请求数据处理
	const int ProcGetSerialTypeData(const std::string &DataType, const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;

	// 请求红酒指数列表（RedwineIndexListTag）
	const int ProcWineIndexList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒指数历史数据(RedwineIndexHistTag)
	const int ProcWineIndexHistory(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;	
	// 请求红酒酒评家及机构数据
	const int ProcWineCritic(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const ;
	// 请求红酒产区分类数据
	const int ProcWineAreaClassified(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒产大产区数据
	const int ProcWineParentArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒产子产区数据
	const int ProcWineChildArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒八大酒庄数据
	const int ProcWineEightWineries(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒销售区域
	const int ProcWineSaleArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒Liv30最新报价列表
	const int ProcWineLiv30Lastest(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒指数成分股数据
	const int ProcWineIndexMem(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒Liv周期为天统计数据
	const int ProcWineLivDayPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒Liv周期为月统计数据
	const int ProcWineLivMonthPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒拍卖日数据
	const int ProcWineAuctionDay(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒拍卖月数据
	const int ProcWineAuctionMonth(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒零售月数据
	const int ProcWineRetailMonth(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求货币单位数据
	const int ProcWineMoneyUnit(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求销售地点数据
	const int ProcWineSaleAddress(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求拍卖行数据
	const int ProcWineAuctionHouse(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求拍卖会日程数据
	const int ProcWineAuctionSchedule(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求名称列表
	const int ProcWineNameList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求容量列表
	const int ProcWineCapacityList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒基本信息列表
	const int ProcWineBaseInfoList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒Liv市场成分酒列表
	const int ProcWineLivMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒拍卖市场成分酒列表
	const int ProcWineAuctionMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒零售市场成分酒列表
	const int ProcWineRetailMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求零售最新报价
	const int ProcWineRetailLatestPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求三个市场酒名称索引列表
	const int ProcWineClassifiedNameListIndex(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒拍卖结果查询结果数据酒标识列表
	const int ProcWineAuctionResSearchMemEnum(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 请求红酒拍卖结果查询关键字
	const int ProcWineAuctionResSearchKey(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 获取拍卖结果全部数据
	const int ProcWineAuctionResSearchData(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// 获取按酒种类归类的拍卖结果全部数据
	const int ProcWineAuctionResByWineTypeData(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;

// public function
public:
	// 初始化
	static int InitialRedWine();
	// 释放资源
	static void ExitRedWineServer();
	// 获取是否已经成功获取了数据库数据
	static bool HaveGotTotalData();
	// 准备取数据环境
	static bool PrepareNetEnv();
	// 释放取数据环境
	static void ReleaseNetEnv();
	// 获取全部静态数据
	static bool RequestAllRemotetData();
	
	/// 外部取数据接口实现(返回0成功)
	// 处理外部请求
	const int ProcRedwineRequest(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	///------------------------------------------------------/

	
};

#endif		/* _INCLUDE_INFOR_REDWINE_H */

