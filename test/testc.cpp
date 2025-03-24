#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>

extern int T, M, N, V, G;
std::vector<int> a;
std::vector<int> start(std::vector<int> b){
    printf("%d",b.at(2));
    return b;
}
int main()
{
    a = {1,2,3};
    auto b = start(std::move(a));
    printf("%d",a.size());
    printf("%d",b.at(2));
    a = {};
    printf("%d",b.at(2));

    return 0;
}