package com.zd.Init;

import java.util.List;

public class ParseMatrix {
    public static int[][] ParseSingleMatrix(List<String> lines, int startIndex, int endIndex, int column_num){
        int[][] fre_matrix = new int[endIndex-startIndex][column_num];
        for(int i=startIndex;i<endIndex;i++){
            String[] line = lines.get(i).trim().split("\\s+");
            for(int j=0;j<column_num;j++){
                fre_matrix[i-startIndex][j] = Integer.parseInt(line[j]);
            }
        }
        return fre_matrix;
    }

    public static String ParseSingleInputMatrix(List<String> lines, int startIndex, int endIndex){
        StringBuilder result = new StringBuilder();
        for(int i=startIndex;i<endIndex;i++){
            String line = lines.get(i).trim();
            char[] char_array = line.toCharArray();
            int slow = 0, fast=0;
            for(;fast<char_array.length;fast++){
                if(char_array[fast] != ' '){
                    if(slow!=0){
                        char_array[slow++] = ' ';
                    }
                    while(fast<char_array.length && char_array[fast] != ' '){
                        char temp = char_array[slow];
                        char_array[slow] = char_array[fast];
                        char_array[fast] = temp;
                        slow++;
                        fast++;
                    }
                }
            }
            result.append(String.valueOf(char_array).trim());
            result.append("\n");
        }
        String matrixs = String.valueOf(result);
        String[] matrix_row = matrixs.split("\n");
        String[] matrix_row0 = matrix_row[0].split(" ");
        int col_num = matrix_row0.length;
        return String.valueOf(result);
    }
}
