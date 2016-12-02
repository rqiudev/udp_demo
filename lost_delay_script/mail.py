#!/usr/bin/env python
#coding: utf-8 import os
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
import os
import sys

def send_mail(receiver, receiver_cc, subject, data_file):
    if not os.path.exists(data_file) : return False

    sender = 'Andrew.liao@uxin.com'
    smtpserver = 'smtp.163.com'
    username = 'uvo_mail@163.com'
    password = 'uvo2014'

    data_text = open(data_file).read()
    #msg = MIMEText(data_text, 'html', 'utf-8')
    #msg['Subject'] = subject
    #msg['From'] = sender
    #msg['To'] = join(receiver)
    #msg['Cc'] = join(receiver_cc)

    print receiver
    print receiver_cc
    msgRoot = MIMEMultipart('related')
    msgRoot['Subject'] = subject
    msgRoot['From'] = sender
    msgRoot['To'] = ','.join(receiver)


    msgText = MIMEText(data_text, 'html', 'utf-8')
    msgRoot.attach(msgText)

    try:
        smtp = smtplib.SMTP()
        smtp.connect(smtpserver)
        smtp.login(username, password)
        smtp.sendmail(username, receiver , msgRoot.as_string())
        smtp.quit()
        return True
    except Exception, e:
        print str(e)
        return False

if __name__ == '__main__':  
    reciver=sys.argv[1]
    reciver_cc=sys.argv[2]
    title=sys.argv[3]
    data_file=sys.argv[4]

    mail_to = "153802088@qq.com, andrew.liao@uxin.com"
   # mail_to = "karman.wu@uxin.com, seven.wu, Kellen.Li@uxin.com, tyler.liu, Vincent.Zhong, yxyw_uvo"
    mail_to = mail_to.split(',')
    mail_to = [ m.strip('\r\n') for m in mail_to ]

    if send_mail(mail_to, mail_to, title, data_file):  
        print "发送成功"  
    else:  
        print "发送失败"  
