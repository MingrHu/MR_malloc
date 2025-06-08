#ifndef CENTRALCACHE_H
#define CENTRALCACHE_H
class CentralCache {
public:
	
	// 单例的唯一全局入口点
	static CentralCache* getInstance() {
		return &_CenInstance;
	}


private:

	CentralCache() = default;
	~CentralCache() = default;
	
	// 禁用拷贝构造 赋值运算
	CentralCache(const CentralCache& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache& copy)noexcept = delete;
	// 禁用移动相关操作
	CentralCache(const CentralCache&& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache&& copy)noexcept = delete;

	static CentralCache _CenInstance;

};

class Span {
	
};
#endif // !CENTRALCACHE_H
