#include <iostream>
#include <tinyxml2.h>

using namespace tinyxml2;
using namespace std;

int xml_read()
{
	XMLDocument doc;
	doc.LoadFile("example.xml");
	XMLElement *root = doc.FirstChildElement("root");
	if (root != nullptr)
	{
		XMLElement *element = root->FirstChildElement("element");
		while (element != nullptr)
		{
			cout << "Element: " << element->GetText() << endl;
			element = element->NextSiblingElement("element");
		}
	}
	return 0;
}

int xml_write()
{
	XMLDocument doc;
	XMLElement *root = doc.NewElement("root");
	doc.InsertFirstChild(root);
	XMLElement *element = doc.NewElement("element");
	element->SetText("value");
	root->InsertEndChild(element);
	doc.SaveFile("example.xml");
	return 0;
}

int xml_add_elem()
{
	XMLDocument doc;
	doc.LoadFile("example.xml");
	XMLElement *root = doc.FirstChildElement("root");
	if (root != nullptr)
	{
		XMLElement *newElement = doc.NewElement("newElement");
		newElement->SetText("newValue");
		root->InsertEndChild(newElement);
		doc.SaveFile("example.xml"); // 保存修改后的文件
	}
	return 0;
}

int xml_del_elem()
{
	XMLDocument doc;
	doc.LoadFile("example.xml");
	XMLElement *root = doc.FirstChildElement("root");
	if (root != nullptr)
	{
		XMLElement *element = root->FirstChildElement("element"); // 假设我们要删除第一个element元素
		if (element != nullptr)
		{
			root->DeleteChild(element); // 删除元素并从父节点中移除它。注意这会删除元素的所有子节点。如果只想删除但不移除，可以使用
										// root->DeleteChild<XMLNode>(element); 代替。
			doc.SaveFile("example.xml"); // 保存修改后的文件。如果要彻底移除但不保存，可以不调用 SaveFile。
		}
	}
	return 0;
}

int main()
{
	xml_read();
	xml_write();
	return 0;
}