

/*===============================================
*   文件名称：linklist.h
*   创 建 者：hejr
*   创建日期：2024年02月22日
*   描    述：
================================================*/
#ifndef __LINKLIST__
#define __LINKLIST__
#include <string.h>

#define DATA_BUFFER_MAXSIZE 1024

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct client_info_s
	{
		char buffer[DATA_BUFFER_MAXSIZE];
		int nlen;
	} data_t;

	typedef struct link_node
	{
		data_t data;			// 数据域
		struct link_node *next; // 指针域
		struct link_node *tail;
	} LinkList;

	// 创建头结点；
	LinkList *Create_Linklist();

	// 判空
	int Linklist_Empty(LinkList *link);

	// 计算表长
	int Linklist_Length(LinkList *link);

	int Linklist_Insert(LinkList *link, char *buffer, int len);

	int Linklist_pop(LinkList *list, data_t *pdata);

	// 从表头插入节点（头插法/尾插法）
	void Linklist_Insert_Head(LinkList *link, data_t data);
	void Linklist_Insert_Front(LinkList *link, data_t data);

	// 按位置插入数据
	void Linklist_Insert_Pos(LinkList *link, int pos, data_t data);

	// 按位置删除数据
	void Linklist_Delete_Pos(LinkList *link, int pos);

	// 按数据删除
	void Linklist_Delete_Data(LinkList *link, data_t data);

	// 按位置查找元素
	int Linklist_Find_Pos(LinkList *link, int pos);

	// 按数据查找元素
	int Linklist_Find_Data(LinkList *link, data_t data);

	// 按值修改元素
	void Linklist_Change_Data(LinkList *link, data_t data, data_t new_data);

	// 清空链表
	void Linklist_Setnull(LinkList *link);

	// 删除链表
	void Linklist_free(LinkList *link);

	// 显示链表
	void Linklist_Show(LinkList *link);

	// 链表的逆序
	LinkList *Order_Linklist(LinkList *link);

	void Order_Linklist_1(LinkList *link);
	void Order_Linklist_2(LinkList *link);

#ifdef __cplusplus
}
#endif

#endif
