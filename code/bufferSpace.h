#include <memory>
#include <stdexcept>
#include <vector>
#include "noncopyable.h"

class Dim1Space:noncopyable{
public:
    void initSpace(int size, int* buffer = nullptr){
        if(buffer == nullptr){
            buffer_ = (int*)malloc(sizeof(int)*size);
        }else{
            buffer_ = buffer;
        }
    }
    int& operator[](int index){
        if(index<0||index>=bufLen){
            throw std::out_of_range("space out of range!");
        }
        //index = index%bufLen;
        return *(buffer_+index);
    }
    ~Dim1Space(){
        free(buffer_);
    }
private:
    int* buffer_ = nullptr;
    int bufLen = 0;
};


//未完成。
class Dim2Space:noncopyable{
public:
    void initSpace(int row, int col, int** buffer = nullptr){
        bufCol = col;
        bufRow = row;
        if(buffer == nullptr){
            int* start = (int*)malloc(sizeof(int)*row*col);
            buffer = (int**)malloc(sizeof(int*)*row);
            for(int i=0;i<row;i++){
                buffer[i] = start;
                start += col;
            }
        }
        dim1Space.push_back(Dim1Space());

    }
    int*& operator[](int row){
        if(row<0||row>=bufRow){
            throw std::out_of_range("space out of range!");
        }
        return *(buffer_+row);
    }
    ~Dim2Space(){
        free(buffer_);

        //delete dim1Space;
    }
private:
    std::vector<Dim1Space> dim1Space;
    int** buffer_;
    int bufRow = 0;
    int bufCol = 0;
};