#include <memory>
#include <stdexcept>
#include <vector>
#include "noncopyable.h"

<<<<<<< HEAD
=======
struct DiskUnit{
    int objId;
    char unitId;
};
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b

template<typename T>
class Dim1Space:noncopyable{
public:
    void initSpace(int size, T* units = nullptr){
        if(units == nullptr){
            units_ = (T*)malloc(sizeof(T)*size);
        }else{
            units_ = units;
        }
        bufLen = size;
    }
    T& operator[](int index){
        if(index<0||index>=bufLen){
            throw std::out_of_range("space out of range!");
        }
        //index = index%bufLen;
        return *(units_+index);
    }
    ~Dim1Space(){
        free(units_);
    }
private:
    T* units_ = nullptr;
    int bufLen = 0;
};


//未完成。
<<<<<<< HEAD
// class Dim2Space:noncopyable{
// public:
//     void initSpace(int row, int col, int** buffer = nullptr){
//         bufCol = col;
//         bufRow = row;
//         if(buffer == nullptr){
//             int* start = (int*)malloc(sizeof(int)*row*col);
//             buffer = (int**)malloc(sizeof(int*)*row);
//             for(int i=0;i<row;i++){
//                 buffer[i] = start;
//                 start += col;
//             }
//         }
//         dim1Space.push_back(Dim1Space<int>());

//     }
//     int*& operator[](int row){
//         if(row<0||row>=bufRow){
//             throw std::out_of_range("space out of range!");
//         }
//         return *(buffer_+row);
//     }
//     ~Dim2Space(){
//         free(buffer_);

//         //delete dim1Space;
//     }
// private:
//     std::vector<Dim1Space<int>> dim1Space;
//     int** buffer_;
//     int bufRow = 0;
//     int bufCol = 0;
// };
=======
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
        dim1Space.push_back(Dim1Space<int>());

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
    std::vector<Dim1Space<int>> dim1Space;
    int** buffer_;
    int bufRow = 0;
    int bufCol = 0;
};
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
