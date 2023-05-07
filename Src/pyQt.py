import sys
from PyQt5.QtChart import QChartView, QBarSeries, QBarSet, QChart, QPieSeries, QBarCategoryAxis, QLineSeries, QValueAxis
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QPainter
from PyQt5.QtWidgets import *
import mysql.connector

class Example(QWidget):

    def __init__(self):
        super().__init__()

        # MariaDB 연결 정보 입력
        self.mydb = mysql.connector.connect(
            host="10.10.141.61",
            user="root",
            password="0000",
            database="iotdb"
        )

        self.chart_view = QChartView(self)
        self.chart_view.setRenderHint(QPainter.Antialiasing)

        self.chart = QChart()
        self.chart.setTitle("Sensor Data")

        self.chart_view.setChart(self.chart)

        # QLineSeries 객체 생성
        self.series1 = QLineSeries()
        self.series2 = QLineSeries()
        self.series3 = QLineSeries()

        # 그래프 객체에 추가
        self.chart.addSeries(self.series1)
        self.chart.addSeries(self.series2)
        self.chart.addSeries(self.series3)

        # X, Y 축 생성
        self.axis_x = QValueAxis()
        self.axis_x.setRange(0, 10)  # 예제에서는 10개의 데이터만 표시
        self.axis_y = QValueAxis()
        self.axis_y.setRange(0, 30)  # 예제에서는 값이 50 이하인 데이터만 있으므로 50으로 설정

        self.chart.addAxis(self.axis_x, Qt.AlignBottom)
        self.chart.addAxis(self.axis_y, Qt.AlignLeft)

        self.series1.attachAxis(self.axis_x)
        self.series1.attachAxis(self.axis_y)

        self.series2.attachAxis(self.axis_x)
        self.series2.attachAxis(self.axis_y)

        self.series3.attachAxis(self.axis_x)
        self.series3.attachAxis(self.axis_y)

        # QLabels 생성
        self.temp_label = QLabel(f"temp: {self.get_count_temp('temp')}", self)
        self.humi_label = QLabel(f"humi: {self.get_count_humi('humi')}", self)
        self.dust_label = QLabel(f"dust: {self.get_count_humi('dust')}", self)

        # QLabels 위치 설정
        self.layout = QVBoxLayout(self)
        self.layout1 = QHBoxLayout(self)

        # 세로
        self.layout.addWidget(self.chart_view)
        self.layout.addLayout(self.layout1)

        # 가로
        self.layout1.addWidget(self.temp_label)
        self.layout1.addWidget(self.humi_label)
        self.layout1.addWidget(self.dust_label)





        # 각 라벨의 스타일시트를 기술합니다.
        self.temp_label.setStyleSheet("background-color: #EDEDED; border-radius: 10%; padding: 10px;")
        self.humi_label.setStyleSheet("background-color: #EDEDED; border-radius: 10%; padding: 10px;")
        self.dust_label.setStyleSheet("background-color: #EDEDED; border-radius: 10%; padding: 10px;")

        self.setStyleSheet("background-color: #D9E5FF; border-radius: 10%;")

        # 각 라벨의 너비를 150으로 설정
        self.temp_label.setFixedWidth(210)
        self.humi_label.setFixedWidth(210)
        self.dust_label.setFixedWidth(210)



        #폰트 설정
        font = self.humi_label.font()
        font = self.temp_label.font()
        font = self.dust_label.font()

        font.setFamily('Arial')
        font.setBold(True)
        self.temp_label.setFont(font)
        self.humi_label.setFont(font)
        self.dust_label.setFont(font)

        # 타이머 생성 및 연결
        self.mytimer = QTimer()
        self.mytimer.timeout.connect(self.update_counts)
        self.mytimer.start(10000)

        self.setGeometry(700, 700, 800, 480) #윈도우 크기
        self.setWindowTitle('Count') # 윈도우 타이틀

    def update_counts(self):
        # MariaDB 연결 정보 입력 재접속
        self.mydb = mysql.connector.connect(
            host="10.10.141.61",
            user="root",
            password="0000",
            database="iotdb"
        )

        latest_temp, latest_humi, latest_dust = self.get_latest_data()
        self.temp_label.setText(f"temp: {latest_temp}")
        self.humi_label.setText(f"humi: {latest_humi}")
        self.dust_label.setText(f"dust: {latest_dust}")

        # 데이터 추가
        x = self.series1.count()  # x축 값
        y1 = latest_temp  # y축 값1
        y2 = latest_humi  # y축 값2
        y3 = latest_dust  # y축 값3

        self.series1.append(x, y1)
        self.series2.append(x, y2)
        self.series3.append(x, y3)

        # 그래프 다시 그리기
        self.axis_x.setRange(max(0, x - 10), x)
        self.axis_y.setRange(0, 30)
        self.chart.update()

    def get_latest_data(self):
        # MariaDB에서 가장 최근 데이터 가져오기
        mycursor = self.mydb.cursor()
        mycursor.execute("SELECT no, temp, humi, dust FROM sensor2 ORDER BY no DESC LIMIT 1")
        row = mycursor.fetchone()
        result = (0, 0, 0, 0)
        if row:
            result = row
        return result[1], result[2], result[3]  # temp, humi, dust 반환

    def get_count_temp(self, temp):
        # MariaDB에서 해당 객체의 개수 가져오기
        mycursor = self.mydb.cursor()
        mycursor.execute(f"SELECT COUNT(*) FROM sensor2 WHERE temp = '{temp}'")
        row = mycursor.fetchone()
        result = 0
        if row:
            result = row[0]
        return result

    def get_count_humi(self, humi):
        # MariaDB에서 해당 객체의 개수 가져오기
        mycursor = self.mydb.cursor()
        mycursor.execute(f"SELECT COUNT(*) FROM sensor2 WHERE humi = '{humi}'")
        row = mycursor.fetchone()
        result = 0
        if row:
            result = row[0]
        return result

    def get_count_dust(self, dust):
        # MariaDB에서 해당 객체의 개수 가져오기
        mycursor = self.mydb.cursor()
        mycursor.execute(f"SELECT COUNT(*) FROM sensor2 WHERE dust = '{dust}'")
        row = mycursor.fetchone()
        result = 0
        if row:
            result = row[0]
        return result
    #윈도우 창
if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = Example()
    ex.show()
    sys.exit(app.exec_())