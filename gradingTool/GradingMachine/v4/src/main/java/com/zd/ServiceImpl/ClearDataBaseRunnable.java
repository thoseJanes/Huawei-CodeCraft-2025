package com.zd.ServiceImpl;

import com.zd.Impl.DiskImpl;
import com.zd.Impl.DiskPointImpl;
import com.zd.Impl.GeneralInformationImpl;
import com.zd.Impl.StorageObjectImpl;

public class ClearDataBaseRunnable implements Runnable{
    private DiskImpl diskImpl = new DiskImpl();
    private DiskPointImpl diskPointImpl = new DiskPointImpl();
    private StorageObjectImpl storageObjectImpl = new StorageObjectImpl();
    private GeneralInformationImpl generalInfoImpl = new GeneralInformationImpl();

    public ClearDataBaseRunnable() {
    }

    @Override
    public void run() {
        System.out.println("数据库清空线程启动"); // 确认线程执行
        diskImpl.clearAll();
        diskPointImpl.clearAll();
        storageObjectImpl.clearAll();
        generalInfoImpl.clearAll();
        System.out.println("数据库清空线程结束，数据库已清空！"); // 确认线程执行结束
    }
}
