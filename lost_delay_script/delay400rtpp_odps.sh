#!/bin/bash

cd `dirname $0`
if [ $# -lt 1 ];then
	echo 'please input max day'
        exit 1
fi

max_day=$1

end_days=`date -d "1 day ago" +'%Y-%m-%d'`
start_days=`date -d "$max_day day ago" +'%Y-%m-%d'`
limitnum=$[max_day*40]

echo "SET odps.stage.mapper.split.size = 32; select ta.dt as dt, td.ip as rtppip,  ta.c400, ta.ott,  if(ta.ott > 0, round(ta.c400/ta.ott*100, 2), 0.00) as rto400, ta.total, if(ta.total > 0, round(ta.c400/ta.total*100, 2), 0.00) as rta400, td.province, td.city, td.provider from (select rb.dt as dt, ra.log_source as ip, sum(if(get_json_object(ra.content, '$.content.tags.call_mode')=1 and (get_json_object(rb.content,'$.callupload[0].ugocallinfo.signaltrace')  not like '%notify%') and split_part(get_json_object(rb.content, '$.callupload[0].qosinfo.delay'), ',', 3) > 400,1,0)) as c400, sum(if(get_json_object(ra.content, '$.content.tags.call_mode')=1 and (get_json_object(rb.content,'$.callupload[0].ugocallinfo.signaltrace')  not like '%notify%'),1,0)) as ott, count(*) as total from ods_uxinapp_rtpp_d ra join ods_uxinapp_realreport_d rb on ra.dt>='$start_days' and ra.dt<='$end_days' and rb.dt>='$start_days' and rb.dt<='$end_days' and get_json_object(rb.content, '$.callupload[0].qosinfo.delay') is not NULL  and  split_part(get_json_object(rb.content, '$.callupload[0].qosinfo.delay'), ',', 2) >= 0.0 and get_calid(get_json_object(rb.content,'$.callupload[0].ugocallinfo.signaltrace'),get_json_object(rb.content,'$.callupload[0].calleephone')) = get_json_object(ra.content, '$.content.tags.c_c_id') group by rb.dt, ra.log_source limit $limitnum) ta join d_ecs_ip_rela tc on tc.inner_ip=ta.ip join ods_ip_info_n td on td.ip=tc.outer_ip order by dt desc, c400 desc limit $limitnum;" | odpscmd


