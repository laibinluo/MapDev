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

typedef RedWineMark* (*FindTypeMemMark)(const int index);				// ���ұ�ʶ����ָ�뺯��

/// ��Ϊ����ָ����ʷ���ݿ��ܲ�����ȡ������������Ѿ�������ͷ
/// ����һ����������ݲ���Ҳ������ͷ
/// �ִ����������������������ʱ����ӱ�ͷ

class CRedWine
{
public:
	CRedWine(void);
	virtual ~CRedWine(void);

// private variable
private:
	// ����������
	static CBlockTcp *m_pSocketServer;
	// �Ƿ�������
	static bool m_bConnected;
	// �����к�
	static int m_PackSerialNum;
	// ��ǰ�����
	static int m_LastestPackSerialNum;
	// ��ǰ��ȡ����ʧ�ܵķ���ֵ
	static int m_iLastestErrorCode;
	
// private function
private:
	// �ַ���ת����unsigned long
	static unsigned long StrToUl(const std::string & data);
	// �ַ���ת����doulbe
	static double StrToDouble(const std::string & data);
	// �ַ�������
	static bool StrNCpy(char *buf, const int StartPos, const int MaxSize, const std::string & data);
	// ת����int
	static int StrToInt(const std::string & data);
	// �Ƿ�Ϊ�մ�
	static bool IsEmptyString(const char* data);
	// ���û��ҵ�λ�������ֶε����λ
	static bool SetHByteMoneyUnitToProperty(const unsigned char MoneyUnitValue, unsigned long &Property);
	// ���þ����������������ֶεĳ����λ��ĵ�λ
	static bool SetExHByteSerialToProperty(const unsigned long MoneyUnitValue, unsigned long &Property);
	// ������������
	static bool SendRequestData(void *ReqData);
	// ��ȡ��Ӧ����
	static int RecvResponseData();
	// ��ȡ�������к�
	static int GetSendSeqNum(const int type = 0);
	// ��ȡ���°�����
	static int GetLastestSeqNum();
	// ���������ʼ��
	static void InitialDatacache();
	// ע��ȡ���ݺ���
	static void RegisterRequestFun();
	// ִ��ȡ���ݺ���
	static void DoRequestStaticFunList();

	// ��ȡ�ض�����������
	// ���͹ؼ��ֶ���: 
	//	'WineIndexList'					ָ���б� 
	//	'WineIndexHistory'				ָ����ʷ����
	//	'RedWineCritic'					�����Ҽ���������
	//	'RedWineAreaClassified'			������������
	//	'RedWineParentArea'				��ƴ��������
	//	'RedWineChildArea'				����Ӳ�������
	//	'EightWineries'					�˴��ׯ�б�
	//	'LivLastestPriceList'			Liv���±����б�
	//	'MemListIndex'					�б�ָ���ɷֹ���������
	//	'RedWineSaleArea'				������������
	//	'WineLivexDayPrice'				Liv�۸�����Ϊ�յ�����
	//	'WineAuctionDayPrice'			�����۸�����Ϊ�յ�����
	//	'LivMonthPriceHisData'			����ͳ��Liv����
	//	'AuctionMonthPriceHisData'		����ͳ����������
	//	'RetailMonthPriceHisData'		����ͳ����������
	//	'RedwineMoneyUnit'				���ҵ�λ����
	//	'RedWineSaleAddress'			������۵ص�����
	//	'RedWineAuctionHouse'			�������������
	//	'RedWineAuctionSchedule'		����������ճ�����
	//	'RedWineCapacityList'			�������
	//	'RedWineNameList'				�������
	//	'RedWineBaseInforList'			��ƻ�����Ϣ
	//	'RedWineLivMemList'				Liv�г���ȫ���ɷ־���Ϣ
	//	'RedWineAutionMemList'			�����г���ȫ���ɷ־���Ϣ
	//	'RedWineRetailMemList'			�����г�ȫ���ɷ־�
	//	'RedwineRetailLatestPrice'		�������±���
	//	'RedWineClassifiedNameListIndex'��ư����г�����������б��б�����
	//	'AuctionResSearchMemEnum'		���������ѯ�Ʊ�ʶ�б�
	//	'AuctionResSearchKeyList'		���������ѯ�ؼ���
	//	'AuctionResSearchDataList'		���������ѯȫ����
	//	'AuctionResByWineTypeDataList'	�������պ����������ѯȫ����
	static const ResponseDataBuf* GetTypeWineData(const std::string &type);

	// �������ָ���б�
	static bool RequestWineIndexList_S();
	// �������ָ���б�
	static bool ParseWineIndexList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	
	// �������ָ��ȫ����
	static bool GetWineIndexData_S();
	// �������ָ����������
	static bool RequestWineIndexClassified(const int TotalCount, const int Serial, const char* KeyWords,
						const int IndexPeriod, CWriteBuff *bufWrite);	
	// �������ָ����ʷ����
	static bool ParseWineIndexClassified(const int TotalCount, const int Serial, const char* KeyWords, 
						void *ReqData, char *data, const int length, CWriteBuff *bufWrite);

	// ��������Ҽ���������
	static bool RequestRedWineCritic_S();
	// ���������Ҽ���������
	static bool ParseRedWineCritic(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ���ľ�����
	static int FindCritic(const std::string &Key);

	// �����Ʋ�����������
	static bool RequestWineAreaClassified_S();
	// ���Ҳ���������������
	static int FindAreaClassified(const std::string &Key);

	// �����ƴ��������
	static bool RequestRedWineParentArea_S();
	// ������ƴ��������
	static bool ParseRedWineParentArea(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ�����������б�����
	static int FindParentArea(const std::string &Key);

	// �������Ӳ�������
	static bool RequestRedWineChildArea_S();
	// ��������Ӳ�������
	static bool ParseRedWineChildArea(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ���Ӳ������б�����
	static int FindChildArea(const std::string &Key);
	
	// ���ɰ˴��ׯ��Ϣ
	static bool GetEightWineriesData_S();

	// ����Liv���������б�
	static bool RequestLivLatestPriceList_S();
	// ����Liv���������б�
	static bool ParseLivLatestPriceList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	
	// ����������������
	static bool RequestSaleArea_S();
	// ������������
	static int FindSaleArea(const std::string &Key);
	
	
	// ����ָֻ���ɷֹ���List�е�����
	static bool RequestSingleIndexMemPosList_S(const int TotalCount, const int Serial, 
						const char* KeyWords, CWriteBuff *bufWrite);
	// ������ָֻ���ɷֹ���List�е�����
	static bool ParseSingleIndexMemPosList_S(const int TotalCount, const int Serial, const char* KeyWords, 
						void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ȫ��ָ���ɷֹ���������
	static bool RequestAllIndexMemPosList_S();

	
	// ��ȡLiv-Ex�۸�������ȫ������
	static bool RequestLivexDayPrice_S();
	// ����Liv-Ex�۸�������ȫ������
	static bool ParseLivexDayPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// �����۸�������ȫ������Դ
	static bool RequestWineAuctionDayPrice_S();
	// ������������������
	static bool ParseWineAuctionDayPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// ������ͳ������
	static bool RequestMonthPrice(const MonthStatmentTag *MarkConditon);
	// ������ͳ������
	static bool ParseMonthPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   const int CmdType, FindTypeMemMark markFun, CWriteBuff *bufWrite, size_t &ItemCount);
	// ����Liv��ͳ������
	static bool ParseLivMonthPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   const int CmdType, FindTypeMemMark markFun, CWriteBuff *bufWrite, size_t &ItemCount);
	// ����ȫ����ͳ������
	static bool RequestMonthPrice_S();

	// ������ҵ�λ����
	static bool ReqeustMoneyUnit_S();
	// ���һ��ҵ�λ
	static int FindMoneyUnit(const std::string &Key);

	// ���������۵ص���Ϣ
	static bool RequestSaleAddress_S();
	// ����������۵ص���Ϣ
	static bool ParseSaleAddress(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// �������۵ص�����
	static int FindSaleAddress(const std::string &Key);
	// ��ȡָ�������۵ص�����������
	static int GetSaleAddressBelongArea(const std::string &Key);

	// ����������������
	static bool RequestAuctionHouse_S();
	// �����������������
	static bool ParseAuctionHouse(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ��������������
	static int FindAuctionHouse(const std::string &Key);

	// �������������ճ�����
	static bool RequestAuctionSchedule_S();
	// ��������������ճ�����
	static bool ParseAuctionSchedule(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);

	// �����������б�����
	static bool RequestRedwineNameList_S();
	// ������������б�����
	static bool ParseRedwineNameList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ���Һ������
	static int FindRedwineName(const std::string &Key); // �����ƴ��� ��Ϊ�ؼ���
	// ��ȡָ������λ�õ�����
	static const RedWineNameS* GetIndexRedWineName(const int index);

	// �����������б�����
	static bool RequestRedwineCapacityList_S();
	// ������������б�����
	static bool ParseRedwineCapacityList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ���Һ������
	static int FindRedwineCapacity(const std::string &Key);	// ����ֵ ��Ϊ�ؼ���
	// ��ȡָ������λ�õ�����
	static const RedwineCapacityS* GetIndexRedWineCapacity(const int index);

	// �������˷���
	static bool RequestRedwineParkRating();
	// �������˷���
	static bool ParseRedwineParkRating(void *ReqData, char *data, const int length);
	// �������˷���
	static const RedwineParkRatinS* FindRedwineParkRating(const std::string &Key); // �����ƴ���+��� ��Ϊ�ؼ���

	// �����ƻ�����Ϣ
	static bool RequestRedwineBaseInfor_S();
	// ������ƻ�����Ϣ
	static bool ParseRedwineBaseInfor(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ���Һ�ƻ�����Ϣ
	static int FindRedwineBaseInfor(const std::string &Key); // �����ƴ��� ��Ϊ�ؼ���
	// ��ȡָ�������ĺ�ƻ�����Ϣ
	static const RedwineBaseInforTag* GetIndexBaseInfor(const int index);

	// ����Liv�г�ȫ���ɷ־�����
	static bool RequestLivMemberList_S();
	// ����Liv�г�ȫ���ɷ־�����
	static bool ParseLivMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ���Ʊ�ʶ������
	static int FindLivMemMarkIndex(const std::string &Key);  // �����ƴ���+���+����+��� ��Ϊ�ؼ���
	// ����ָ���Ƶı�ʶID
	static RedWineMark* FindLivMemMark(const int index);
	// ����Liv�г���Ա����������ʷ������Ŀ
	static bool RequestLivMemberDayPriceElement();
	// ����Liv�г���Ա����������ʷ������Ŀ
	static bool ParseLivMemberDayPriceElement(void *ReqData, char *data, const int length);
	// ����Liv�г���Ա����������ʷ������Ŀ
	static bool RequestLivMemberMonthPriceElement();
	// ����Liv�г���Ա����������ʷ������Ŀ
	static bool ParseLivMemberMonthPriceElement(void *ReqData, char *data, const int length);

	// ���������г�ȫ���ɷ־�����
	static bool RequestAutionMemberList_S();
	// ���������г�ȫ���ɷ־�����
	static bool ParseAuctionMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ���Ʊ�ʶ������
	static int FindAuctionMemMarkIndex(const std::string &Key);  // �����ƴ���+���+���� ��Ϊ�ؼ���
	// ����ָ���Ƶı�ʶID
	static RedWineMark* FindAuctionMemMark(const int index);
	// ���������г���Ա����������ʷ������Ŀ
	static bool RequestAuctionMemberDayPriceElement();
	// ���������г���Ա����������ʷ������Ŀ
	static bool ParseAuctionMemberDayPriceElement(void *ReqData, char *data, const int length);
	// ���������г���Ա����������ʷ������Ŀ
	static bool RequestAuctionMemberMonthPriceElement();
	// ���������г���Ա����������ʷ������Ŀ
	static bool ParseAuctionMemberMonthPriceElement(void *ReqData, char *data, const int length);
	
	// ���������г�ȫ���ɷ־�����(�Ѿ�����ͳ����ͳ����Ϣ)
	static bool RequestRetailMemberList_S();
	// ���������г�ȫ���ɷ־�����
	static bool ParseRetailMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ���Ʊ�ʶ������
	static int FindRetailMemMarkIndex(const std::string &Key);  // �����ƴ���+���+����+���� ��Ϊ�ؼ���
	// ����ָ���Ƶı�ʶID
	static RedWineMark* FindRetailMemMark(const int index);
	// �����������±���
	static bool RequestRetailLatestPrice_S();
	// �����������±���
	static bool ParseRetailLatestPrice(int &Serial, void *ReqData, char *data, const int length, CWriteBuff *bufWrite, size_t &ItemCount);

	// �����������г������������б�
	static bool RequestClassifiedNameListIndex_S();
	// ������������г������������б�
	static bool ParseClassifiedNameListIndex(const int TotalCount, int &Serial, void *ReqData, char *data, 
		const int length, CWriteBuff *bufWrite);

	// ���������������ѯ������ݾƱ�ʶ�б�
	static bool RequestAuctionResSearchMemEnum_S();
	// ����������������ѯ������ݾƱ�ʶ�б�
	static bool ParseAuctionResSearchMemEnum(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// ����ָ���Ʊ�ʶ������
	static int FindAuctionResSearchMemMarkIndex(const std::string &Key);  // �����ƴ���+���+���� ��Ϊ�ؼ���
	// ��������ָ���Ʊ�ʶ
	static RedWineMark* FindAuctionResSearchMemMark(const int index);

	// ���������������ѯ�ؼ��� (��������ͳ��)
	static bool RequestAuctionResSearchKey_S();
	// ����������������ѯ�ؼ���(�ص�+��������)���
	static bool ParseAuctionResSearchKey(void *ReqData, char *data, const int length, CWriteBuff *bufWrite);
	// �������������ѯ�ؼ��ֵ�����
	static int FindAuctionResSearchKeyIndex(const std::string &Key);  // �����ص�+������ ��Ϊ�ؼ���
	// �������������ѯ�ؼ���ID
	static RedwineAuctionResMark* FindAuctionResSearchKey(const int index);

	// ��ȡ�������ȫ������
	static bool RequestAuctionResSearchData_S();
	// �����������ȫ������
	static bool ParseAuctionResSearchData(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// ��������������վ����໮�ֵ�ȫ������
	static bool RequestAuctionResSingleTypeData_S();
	// ��������������վ����໮�ֵ�ȫ������
	static bool ParseAuctionResSingleTypeData(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
				   CWriteBuff *bufWrite, size_t &ItemCount);

	// ����������ݲ����ڵ�ʱ�򣬲���հ�ͷ
	static bool AddEmptyHeadData(const int TotalCount, const int Serial, const int HeadCmdType, CWriteBuff *bufWrite);

private:
	// һ���������ݴ���
	const int ProcGetTotalTypeData(const std::string &DataType, const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �ִ��������ݴ���
	const int ProcGetSerialTypeData(const std::string &DataType, const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;

	// ������ָ���б�RedwineIndexListTag��
	const int ProcWineIndexList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������ָ����ʷ����(RedwineIndexHistTag)
	const int ProcWineIndexHistory(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;	
	// �����ƾ����Ҽ���������
	const int ProcWineCritic(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const ;
	// �����Ʋ�����������
	const int ProcWineAreaClassified(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����Ʋ����������
	const int ProcWineParentArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����Ʋ��Ӳ�������
	const int ProcWineChildArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����ư˴��ׯ����
	const int ProcWineEightWineries(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ��������������
	const int ProcWineSaleArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������Liv30���±����б�
	const int ProcWineLiv30Lastest(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������ָ���ɷֹ�����
	const int ProcWineIndexMem(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������Liv����Ϊ��ͳ������
	const int ProcWineLivDayPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������Liv����Ϊ��ͳ������
	const int ProcWineLivMonthPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ����������������
	const int ProcWineAuctionDay(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ����������������
	const int ProcWineAuctionMonth(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ����������������
	const int ProcWineRetailMonth(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������ҵ�λ����
	const int ProcWineMoneyUnit(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �������۵ص�����
	const int ProcWineSaleAddress(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ��������������
	const int ProcWineAuctionHouse(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����������ճ�����
	const int ProcWineAuctionSchedule(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ���������б�
	const int ProcWineNameList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ���������б�
	const int ProcWineCapacityList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����ƻ�����Ϣ�б�
	const int ProcWineBaseInfoList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ������Liv�г��ɷ־��б�
	const int ProcWineLivMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����������г��ɷ־��б�
	const int ProcWineAuctionMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����������г��ɷ־��б�
	const int ProcWineRetailMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// �����������±���
	const int ProcWineRetailLatestPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ���������г������������б�
	const int ProcWineClassifiedNameListIndex(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ���������������ѯ������ݾƱ�ʶ�б�
	const int ProcWineAuctionResSearchMemEnum(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ���������������ѯ�ؼ���
	const int ProcWineAuctionResSearchKey(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ��ȡ�������ȫ������
	const int ProcWineAuctionResSearchData(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	// ��ȡ�������������������ȫ������
	const int ProcWineAuctionResByWineTypeData(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;

// public function
public:
	// ��ʼ��
	static int InitialRedWine();
	// �ͷ���Դ
	static void ExitRedWineServer();
	// ��ȡ�Ƿ��Ѿ��ɹ���ȡ�����ݿ�����
	static bool HaveGotTotalData();
	// ׼��ȡ���ݻ���
	static bool PrepareNetEnv();
	// �ͷ�ȡ���ݻ���
	static void ReleaseNetEnv();
	// ��ȡȫ����̬����
	static bool RequestAllRemotetData();
	
	/// �ⲿȡ���ݽӿ�ʵ��(����0�ɹ�)
	// �����ⲿ����
	const int ProcRedwineRequest(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const;
	///------------------------------------------------------/

	
};

#endif		/* _INCLUDE_INFOR_REDWINE_H */

