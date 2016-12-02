#!/bin/bash
cd `dirname $0`

#设置最大统计天数
max_statis_day=10


#丢包统计
#直拨丢包落地网关统计
lost_tmp_file=./lost.log
if test -f $lost_tmp_file;then
	rm -f $lost_tmp_file
fi
./lost_odps.sh $max_statis_day > $lost_tmp_file
sed -i -e '/^odps/d' -e  '/^$/d' -e '/^|.*dates.*/d' -e '/^\+\-*/d' -e  's/[\|]/ /g' -e 's/,/ /g' $lost_tmp_file

#直拔丢包最多的落地网关的时间段统计
lost_gw_time_tmp_file=./lost_gw_time.log
if test -f $lost_gw_time_tmp_file;then
        rm -f $lost_gw_time_tmp_file
fi
./lost_max_gw_time_odps.sh 3 > $lost_gw_time_tmp_file
sed -i -e '/^odps/d' -e '/\+\-*/d' -e '/[|][ \t]*dt/d'  -e '/^$/d' -e 's/[\|]//g' -e 's/,/ /g' $lost_gw_time_tmp_file

#直拔丢包最多的落地网关的RTPP统计
lost_gw_rtpp_tmp_file=./lost_gw_rtpp.log
if test -f $lost_gw_rtpp_tmp_file;then
        rm -f $lost_gw_rtpp_tmp_file
fi
./lost_max_gw_rtpp_odps.sh 3 > $lost_gw_rtpp_tmp_file
sed -i -e '/^odps/d' -e '/\+\-*/d' -e '/[|][ \t]*dt/d'  -e '/^$/d' -e 's/[\|]//g' -e 's/,/ /g' $lost_gw_rtpp_tmp_file

#延时统计
#延迟(400ms)的网络状态统计
delay_tmp_file=./delay.log
if test -f $delay_tmp_file;then
    rm -f $delay_tmp_file
fi
./delay_odps.sh 5 > $delay_tmp_file
sed -i -e '/^odps/d' -e '/\+\-*/d' -e '/[|][ \t]*dt/d'  -e '/^$/d' -e 's/[\|]//g' -e 's/,/ /g' $delay_tmp_file

#延迟(400ms)的RTPP统计
delay_rtpp_tmp_file=./delay_rtpp.log
if test -f $delay_rtpp_tmp_file;then
        rm -f $delay_rtpp_tmp_file
fi
./delay400rtpp_odps.sh 3 > $delay_rtpp_tmp_file
sed -i -e '/^odps/d' -e '/\+\-*/d' -e '/[|][ \t]*dt/d'  -e '/^$/d' -e 's/[\|]//g' -e 's/,/ /g' $delay_rtpp_tmp_file

#延迟(400ms)最多的RTPP的时间段统计
delay_timestat_tmp_file=./delay_timestat.log
if test -f $delay_timestat_tmp_file;then
        rm -f $delay_timestat_tmp_file
fi
./delay400timestat_odps.sh 3 > $delay_timestat_tmp_file
sed -i -e '/^odps/d' -e '/\+\-*/d' -e '/[|][ \t]*dt/d'  -e '/^$/d' -e 's/[\|]//g' -e 's/,/ /g' $delay_timestat_tmp_file

#延迟(400ms)最多的RTPP测试包的时间段统计
delay_max_rtpp_test_time_tmp_file=./delay_max_rtpp_test_time.log
if test -f $delay_max_rtpp_test_time_tmp_file;then
        rm -f $delay_max_rtpp_test_time_tmp_file
fi
./delay_max_rtpp_test_time_odps.sh 3 > $delay_max_rtpp_test_time_tmp_file
sed -i -e '/^odps/d' -e '/\+\-*/d' -e '/[|][ \t]*dt/d'  -e '/^$/d' -e 's/[\|]//g' -e 's/,/ /g' $delay_max_rtpp_test_time_tmp_file

html_file=./send.html
if test -f $html_file;then
	rm -f $html_file
fi

echo '<html><body>' >> $html_file


# 开始构造直拨丢包的html表格
echo '<h1>直拨丢包统计</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="40">日期(s1)</td><td class='onecenter' width="45">IP地址(s2)</td><td class='onecenter' width="150">IP丢包大于15%数量(s3)</td><td class='onecenter' width="150">IP请求总数(s4)</td><td class='onecenter' width="150">所有IP丢包大于15%的请求总数(s5)</td><td class='onecenter' width="150">IP丢包大于15%占本IP的百分比(s3/s4)</td><td class='onecenter' width="150">IP丢包大于15%占总请求量中大于15%的百分比(s3/s5)</td>' >> $html_file

#color_list=(BBFFFF 00F5FF 54FF9F 00FF00 B0C4DE 54FF9F 8470FF FFD39B B9D3EE 00BFFF)
color_list=(BBFFFF 00F5FF BBFFFF 00F5FF BBFFFF 00F5FF BBFFFF 00F5FF BBFFFF 00F5FF)
suffix=0
prev=''
cat $lost_tmp_file |
while read dt ip ip_15_cnt ip_all_cnt all_15_cnt ip_pre all_pre
do	
	if [ -z $prev ];then
            prev=$dt
	fi

	if [ -X"$prev" != -X"$dt" ];then
	    suffix=`echo $(($suffix + 1))`
	    prev=$dt
	    if [ $suffix -gt 10 ];then
		suffix=0
	    fi
	fi

	clr=${color_list[$suffix]}
        
	echo "<tr align=\"center\">" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$ip</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$ip_15_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$ip_all_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$all_15_cnt</td>" >> $html_file
	selfclr=$clr
        val=`echo "$ip_pre" | awk -F '.' '{print $1}'`
        if [ $val -ge 10 ];then
            selfclr=FF0000
        elif [ $val -ge 5 ];then
            selfclr=FFFF00
        fi

	echo "<td class='onecenter' bgcolor=$selfclr>${ip_pre}%</td>" >> $html_file

	allclr=$clr
	val=`echo "$all_pre" | awk -F '.' '{print $1}'`
	if [ $val -ge 10 ];then
	    allclr=FF0000
	elif [ $val -ge 5 ];then
            allclr=FFFF00
	fi
	echo "<td class='onecenter' bgcolor=$allclr>${all_pre}%</td>" >> $html_file
	echo "</tr>" >> $html_file
done

#完成直拔丢包的统计报表
echo '</table>' >> $html_file

#开始直拔丢包最多的落地网关的时间段统计报表
echo '<h1>直拨丢包最多落地网关时间段统计报表</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="150">日期</td><td class='onecenter' width="45">落地网关地址</td><td class='onecenter' width="150">时间段</td><td class='onecenter' width="150">当前时间段IP丢包大于15%数量</td><td class='onecenter' width="150">当前时间段IP请求总数</td><td class='onecenter' width="150">当前时间段IP丢包大于15%数量占总量的百分比</td>' >> $html_file


cat $lost_gw_time_tmp_file |
while read dt gwip hr cnt15 total rate15
do
        echo "<tr align=\"center\">" >> $html_file
        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file

        clr=EECBAD
        echo "<td class='onecenter' bgcolor=$clr>$gwip</td>" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$hr</td>" >> $html_file


        clr_val=CCCCCC
        clr=FFFF00
        echo "<td class='onecenter' bgcolor=$clr>$cnt15</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$total</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rate15%</td>" >> $html_file

        echo "</tr>" >> $html_file
done
#完成直拔丢包最多的落地网关的RTPP统计报表
echo '</table>' >> $html_file

#开始直拔丢包最多的落地网关的RTPP统计报表
echo '<h1>直拨丢包最多落地网关RTPP统计报表</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="150">日期</td><td class='onecenter' width="45">落地网关地址</td><td class='onecenter' width="150">RTPP地址</td><td class='onecenter' width="150">RTPP所在城市</td><td class='onecenter' width="150">当前RTPP的IP丢包大于15%数量</td><td class='onecenter' width="150">当前RTPP的IP请求总数</td><td class='onecenter' width="150">当前RTPP的IP丢包大于15%数量占总量的百分比</td>' >> $html_file


cat $lost_gw_rtpp_tmp_file |
while read dt gwip rtppip city cnt15 total rate15
do
        echo "<tr align=\"center\">" >> $html_file
        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file

        clr=EECBAD
        echo "<td class='onecenter' bgcolor=$clr>$gwip</td>" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$rtppip</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$city</td>" >> $html_file


        clr_val=CCCCCC
        clr=FFFF00
        echo "<td class='onecenter' bgcolor=$clr>$cnt15</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$total</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rate15%</td>" >> $html_file

        echo "</tr>" >> $html_file
done
#完成直拔丢包最多的落地网关的RTPP统计报表
echo '</table>' >> $html_file



#开始延迟(400ms)的统计html表格
echo '<h1>OTT延迟(大于400)统计报表</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="40">日期</td><td class='onecenter' width="45">延迟大于400ms总数</td><td class='onecenter' width="150">单通总量</td><td class='onecenter' width="150">单通百分比</td><td class='onecenter' width="150">多网络切换总数</td><td class='onecenter' width="150">多网络切换百分比</td><td class='onecenter' width="150">使用2G</td><td class='onecenter' width="150">使用2G百分</td><td class='onecenter' width="150">使用移动3G</td><td class='onecenter' width="150">使用移动3G百分比</td><td class='onecenter' width="150">仅使用(电信/联通)3G</td><td class='onecenter' width="150">仅使用(电信/联通)3G百分比</td><td class='onecenter' width="150">仅使用4G</td><td class='onecenter' width="150">仅使用4G百分比</td><td class='onecenter' width="150">仅使用WIFI</td><td class='onecenter' width="150">仅使用WIFI百分比</td><td class='onecenter' width="150">仅使用3G(电信/联通)/4G/WIFI</td><td class='onecenter' width="150">仅使用3G(电信/联通)/4G/WIFI百分比</td>' >> $html_file

cat $delay_tmp_file |
while read dt total sig_cnt sig_pre xchg_cnt xch_pre u2g_cnt u2g_pre uyd_3g_cnt uyd_3g_pre u3g_cnt u3g_pre u4g_cnt u4g_pre uwifi_cnt uwifi_pre ok_cnt ok_pre
do
	echo "<tr align=\"center\">" >> $html_file

	clr=B9D3EE
	echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file

	clr=EECBAD
	echo "<td class='onecenter' bgcolor=$clr>$total</td>" >> $html_file

	clr_val=CCCCCC
	clr=FFFF00
	echo "<td class='onecenter' bgcolor=$clr_val>$sig_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$sig_pre%</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr_val>$xchg_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$xch_pre%</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr_val>$u2g_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$u2g_pre%</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr_val>$uyd_3g_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$uyd_3g_pre%</td>" >> $html_file

	clr=00FF7F
	echo "<td class='onecenter' bgcolor=$clr_val>$u3g_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$u3g_pre%</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr_val>$u4g_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$u4g_pre%</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr_val>$uwifi_cnt</td>" >> $html_file
	echo "<td class='onecenter' bgcolor=$clr>$uwifi_pre%</td>" >> $html_file

	clr=00FF00
	echo "<td class='onecenter' bgcolor=$clr_val>$ok_cnt</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$ok_pre%</td>" >> $html_file
	

	echo "</tr>" >> $html_file
done
#完成延迟(400ms)的统计报表
echo '</table>' >> $html_file

#开始延迟(400ms)的RTPP统计html表格
echo '<h1>OTT延迟(大于400)RTPP统计报表</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="150">日期</td><td class='onecenter' width="45">RTPP地址</td><td class='onecenter' width="150">延迟大于400ms的OTT总数</td><td class='onecenter' width="150">OTT总量</td><td class='onecenter' width="150">延迟大于400ms占OTT百分比</td><td class='onecenter' width="150">会话总量</td><td class='onecenter' width="150">延迟大于400ms占会话总量百分比</td><td class='onecenter' width="150">省份</td><td class='onecenter' width="150">城市</td><td class='onecenter' width="150">供应商</td>' >> $html_file


cat $delay_rtpp_tmp_file |
while read dt rtppip c400 ott rto400 total rta400 province city provider
do
        echo "<tr align=\"center\">" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file

        clr=EECBAD
        echo "<td class='onecenter' bgcolor=$clr>$rtppip</td>" >> $html_file

        clr_val=CCCCCC
        clr=FFFF00
        echo "<td class='onecenter' bgcolor=$clr>$c400</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$ott</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rto400%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$total</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rta400%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$province</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$city</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$provider</td>" >> $html_file

        echo "</tr>" >> $html_file
done
#完成延迟(400ms)的RTPP统计报表
echo '</table>' >> $html_file

#开始延迟(400ms)的RTPP时间段统计html表格
echo '<h1>OTT延迟(大于400)RTPP时间段统计报表</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="150">日期</td><td class='onecenter' width="45">RTPP地址</td><td class='onecenter' width="150">时间点(小时)</td><td class='onecenter' width="150">延迟大于400ms的会话数</td><td class='onecenter' width="150">会话总量</td><td class='onecenter' width="150">延迟大于400ms的会话数占会话总量的百分比</td><td class='onecenter' width="150">该RTPP在当前时间段的平均并发会话数</td>' >> $html_file
cat $delay_timestat_tmp_file |
while read dt rtppip hr c400 total r400 sm
do
        echo "<tr align=\"center\">" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file

        clr=EECBAD
        echo "<td class='onecenter' bgcolor=$clr>$rtppip</td>" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$hr</td>" >> $html_file

        clr_val=CCCCCC
        clr=FFFF00
        echo "<td class='onecenter' bgcolor=$clr>$c400</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$total</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$r400%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$sm</td>" >> $html_file

        echo "</tr>" >> $html_file
done
#完成延迟(400ms)的RTPP时间段统计html表格
echo '</table>' >> $html_file

#开始延迟(400ms)的RTPP测试包时间段统计html表格
echo '<h1>OTT延迟(大于400)RTPP测试包时间段统计报表</h1><table border="1" cellspacing="0" cellpadding="0" style="margin:0px auto">' >> $html_file
echo '<tr align="center" bgcolor=B9D3EE><td class='onecenter' width="150">日期</td><td class='onecenter' width="45">RTPP地址</td><td class='onecenter' width="150">时间点(小时)</td><td class='onecenter' width="150">延迟大于400ms的会话数</td><td class='onecenter' width="150">会话总量</td><td class='onecenter' width="150">延迟大于400ms的会话数占会话总量的百分比</td><td class='onecenter' width="150">该RTPP在当前时间段的平均并发会话数</td><td class='onecenter' width="150">测试机发送给该RTPP在当前时间段的PING包RTT平均时间</td><td class='onecenter' width="150">该RTPP在当前时间段收到的测试包数</td><td class='onecenter' width="150">该RTPP在当前时间段的收到的测试包的平均时间</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为0-25ms的测试包数</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为0-25ms的测试包数占总测试包的百分比</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为25-50ms的测试包数</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为25-250ms的测试包数占总测试包的百分比</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为50-100ms的测试包数</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为50-100ms的测试包数占总测试包的百分比</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为100-400ms的测试包数</td><td class='onecenter' width="150">该RTPP在当前时间段的延时为100-400ms的测试包数占总测试包的百分比</td><td class='onecenter' width="150">该RTPP在当前时间段的延时大于400ms的测试包数</td><td class='onecenter' width="150">该RTPP在当前时间段的延时大于400ms的测试包数占总测试包的百分比</td>' >> $html_file
cat $delay_max_rtpp_test_time_tmp_file |
while read dt rtppip hr c400 stotal r400 sm rtt pcnt0 tave0  delay0   rdelay0  delay1   rdelay1  delay2   rdelay2  delay3   rdelay3  delay4  rdelay4 
do
        echo "<tr align=\"center\">" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$dt</td>" >> $html_file

        clr=EECBAD
        echo "<td class='onecenter' bgcolor=$clr>$rtppip</td>" >> $html_file

        clr=B9D3EE
        echo "<td class='onecenter' bgcolor=$clr>$hr</td>" >> $html_file

        clr_val=CCCCCC
        clr=FFFF00
        echo "<td class='onecenter' bgcolor=$clr>$c400</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$stotal</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$r400%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$sm</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rtt</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$pcnt0</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$tave0</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$delay0</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rdelay0%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$delay1</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rdelay1%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$delay2</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rdelay2%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$delay3</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rdelay3%</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr_val>$delay4</td>" >> $html_file
        echo "<td class='onecenter' bgcolor=$clr>$rdelay4%</td>" >> $html_file

        echo "</tr>" >> $html_file
done
#完成延迟(400ms)的RTPP测试包时间段统计html表格
echo '</table>' >> $html_file

echo '</body></html>' >> $html_file
./mail.py "['glory.ling@uxin.com', 'vic.qiu@uxin.com', 'andrew.liao@uxin.com']" "['glory.ling@uxin.com', 'vic.qiu@uxin.com']" "[监控]-落地网关丢包统计(`date +'%Y-%m-%d'`)" $html_file


