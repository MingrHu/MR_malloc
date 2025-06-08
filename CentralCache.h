#ifndef CENTRALCACHE_H
#define CENTRALCACHE_H
class CentralCache {
public:
	
	// ������Ψһȫ����ڵ�
	static CentralCache* getInstance() {
		return &_CenInstance;
	}


private:

	CentralCache() = default;
	~CentralCache() = default;
	
	// ���ÿ������� ��ֵ����
	CentralCache(const CentralCache& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache& copy)noexcept = delete;
	// �����ƶ���ز���
	CentralCache(const CentralCache&& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache&& copy)noexcept = delete;

	static CentralCache _CenInstance;

};

class Span {
	
};
#endif // !CENTRALCACHE_H
