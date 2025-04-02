package com.zd.Util;

public class GeneralInformation {
    public double Scores;
    public int totalRequests;
    public int abortedRequests;
    public int finishedRequests;
    public int timeStamp;

    public GeneralInformation() {
    }

    public GeneralInformation(int timeStamp, double scores, int totalRequests, int abortedRequests, int finishedRequests) {
        Scores = scores;
        this.totalRequests = totalRequests;
        this.abortedRequests = abortedRequests;
        this.finishedRequests = finishedRequests;
        this.timeStamp = timeStamp;
    }
}
