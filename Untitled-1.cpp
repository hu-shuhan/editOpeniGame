#include <iostream>
#include<vector>
using namespace std;

int main() {
    int day,nums;
    float start;
    cin>>day>>nums>>start;
    vector<vector<float>>price(day,vector<float>(nums));
    vector<vector<int>>op(day,vector<int>(2));
    int buy_index=-1;
    float buy_nums;
    for(int i = 0;i < day; ++i){
        if((i-1)>=0&&buy_index!=-1){
            start+=buy_nums*price[i][buy_index];
            op[i][0]=buy_index;
        }
        else{
            op[i][0]=buy_index;
        }
        buy_index=-1;
        if((i+1)<day){
            float bouns=1;
            for(int j=0;j<nums;++j){
                float tem=price[i+1][j]/price[i][j];
                if(tem>bouns){
                    buy_index=j;
                    bouns=tem;
                }
            }
            if(bouns!=1){
                buy_nums=start/price[i][buy_index];
                start=0;
                op[i][1]=buy_index;
            }
            else{
                op[i][1]=-1;
            }
        }
    }
    printf("%.4f",start);
    for(int i=0;i<day;++i){
        cout<<op[i][0]<<' '<<op[i][1]<<endl;
    }
}
// 64 位输出请用 printf("%lld")