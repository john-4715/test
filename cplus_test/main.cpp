
#include <iostream>
#include <vector>

using namespace std;
int main()
{
	std::vector<int> v1;
	v1.reserve(10); // 容器预留 len 个元素长度
	cout << "v1初始 size 和 capacity大小:\n " << "\tv1_size : " << v1.size() << "\t v1_capacity: " << v1.capacity()
		 << endl
		 << endl;

	// 添加元素
	cout << "v1c中插入元素后 size 和 capacity大小: " << v1.size() << endl << endl;
	for (int i = 0; i < 23; i++)
	{
		v1.push_back(i);
		cout << "\tv1_size: " << v1.size() << "\tv1_capacity: " << v1.capacity() << endl;
	}

	return 0;
}
