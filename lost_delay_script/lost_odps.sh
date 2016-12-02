#!/bin/bash

cd `dirname $0`
if [ $# -lt 1 ];then
	echo 'please input max day'
        exit 1
fi


max_day=$1

#for ((i=1; i<=$max_day; ++i))
#do
#	tday=`date -d "$i day ago" +'%Y-%m-%d'`
	
#	#tablename=t_andrew_liao_tmp_${i}_day_`date +%s`
#	echo "select t1.dates,t1.ipaddr,t1.total_ip_15_cnt,t1.total_ip_cnt,t2.total_15_cnt, round((t1.total_ip_15_cnt/t1.total_ip_cnt)*100, 2), round((t1.total_ip_15_cnt/t2.total_15_cnt)*100,2) from (SELECT dt AS dates, get_json_object(content, '$.content.tags.callee_pkg_addr[0]') AS ipaddr, sum(if(get_json_object(content, '$.content.tags.callee_seq[4]')>15,1,0)) as total_ip_15_cnt, count(*) AS total_ip_cnt FROM ods_uxinapp_rtpp_d WHERE dt='$tday' AND get_json_object(content, '$.content.tags.call_mode')=3 GROUP BY dt, get_json_object(content, '$.content.tags.callee_pkg_addr[0]')) t1 inner join (SELECT dt AS dates,sum(if(get_json_object(content, '$.content.tags.callee_seq[4]')>15,1,0)) as total_15_cnt FROM ods_uxinapp_rtpp_d WHERE dt='$tday' AND get_json_object(content, '$.content.tags.call_mode')=3 GROUP BY dt) t2 on t1.dates=t2.dates ORDER BY t1.total_ip_15_cnt DESC LIMIT 10;" | odpscmd 
     	#echo "tunnel download $tablename ./${filename}.$i;" | odpscmd
	#echo "drop table $tablename;" | odpscmd
#done

end_days==`date +'%Y-%m-%d'`
start_days=`date -d "$max_day day ago" +'%Y-%m-%d'`

echo "SET odps.stage.mapper.split.size = 32; select tt.dates, tt.ipaddr, tt.total_ip_15_cnt, tt.total_ip_cnt, tt.total_15_cnt, tt.total_ip_pre, tt.total_15_pre from (SELECT t1.dates as dates, t1.ipaddr as ipaddr, t1.total_ip_15_cnt as total_ip_15_cnt, t1.total_ip_cnt as total_ip_cnt, t2.total_15_cnt as total_15_cnt, round((t1.total_ip_15_cnt/t1.total_ip_cnt)*100, 2) as total_ip_pre, round((t1.total_ip_15_cnt/t2.total_15_cnt)*100,2) as total_15_pre, rank() over (partition by t1.dates order by t1.total_ip_15_cnt desc) as rank FROM (SELECT dt AS dates, get_json_object(content, '$.content.tags.callee_pkg_addr[0]') AS ipaddr, sum(if(get_json_object(content, '$.content.tags.callee_seq[4]')>15,1,0)) AS total_ip_15_cnt, count(*) AS total_ip_cnt FROM ods_uxinapp_rtpp_d WHERE dt>='$start_days' and dt<'$end_days' AND get_json_object(content, '$.content.tags.call_mode')=3 GROUP BY dt, get_json_object(content, '$.content.tags.callee_pkg_addr[0]')) t1 INNER JOIN (SELECT dt AS dates, sum(if(get_json_object(content, '$.content.tags.callee_seq[4]')>15,1,0)) AS total_15_cnt FROM ods_uxinapp_rtpp_d WHERE dt>='$start_days' and dt<='$end_days' AND get_json_object(content, '$.content.tags.call_mode')=3 GROUP BY dt) t2 ON t1.dates=t2.dates) tt where tt.rank<=10 order by tt.dates desc limit 200;" | odpscmd


