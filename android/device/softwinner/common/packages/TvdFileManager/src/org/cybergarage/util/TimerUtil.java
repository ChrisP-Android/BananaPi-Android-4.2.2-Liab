package org.cybergarage.util;

public final class TimerUtil
{
	/** 设置线程的睡眠
	 * @param waitTime 睡眠时间
	 *  
	 */
	public final static void wait(int waitTime)
	{
		try {
			Thread.sleep(waitTime);
		}
		catch (Exception e) {}
	}

	/** 线程随机时间睡眠
	 * @param  time 睡眠时间×随机数 等于线程的睡眠时间
	 */
	public final static void waitRandom(int time)
	{
		int waitTime = (int)(Math.random() * time);		
		try {
			Thread.sleep(waitTime);
		}
		catch (Exception e) {}
	}
}

