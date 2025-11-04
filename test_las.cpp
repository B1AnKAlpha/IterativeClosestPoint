#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;

int main() {
    system("chcp 65001>nul");
    cout << "测试程序开始..." << endl;
    
    ifstream file("Scannew_096.las", ios::binary);
    if (!file.is_open()) {
        cout << "无法打开文件!" << endl;
    } else {
        cout << "文件打开成功!" << endl;      
        
        // 读取文件头
        char header[227];
        file.read(header, 227);
        
        cout << "文件签名: ";
        for (int i = 0; i < 4; i++) {
            cout << header[i];
        }
        cout << endl;
        
        file.close();
    }
    
    cout << "\n测试完成!" << endl;
    cout << "按回车键退出..." << endl;
    cin.get();  // 等待用户按回车
    return 0;
}
