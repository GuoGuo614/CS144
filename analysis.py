import re
from datetime import datetime
import time
import matplotlib.pyplot as plt

import numpy as np
import matplotlib.dates as mdates

rtts = []
timestamps = []

# 1. Parse the ping file
filename = "data.txt"
seqs = set()

with open(filename, 'r') as f:
    for line in f:
        # Match lines like "... icmp_seq=X ..."
        m = re.search(r'icmp_seq=(\d+)', line)
        m_ts = re.search(r'\[(\d+\.\d+)\]', line)
        m_rtt = re.search(r'time=([\d\.]+) ms', line)

        if m:
            seqs.add(int(m.group(1)))
        if m_ts and m_rtt:
            # Parse Unix timestamp
            ts_float = float(m_ts.group(1))
            timestamps.append(datetime.fromtimestamp(ts_float))
            
            # Parse RTT
            rtt_float = float(m_rtt.group(1))
            rtts.append(rtt_float)

if not seqs:
    print("No sequence numbers found.")
    exit()

min_seq = min(seqs)
max_seq = max(seqs)
total_sent = max_seq - min_seq + 1

# Create a boolean array where True = received, False = lost
# Index i roughly corresponds to min_seq + i
received = [ (seq in seqs) for seq in range(min_seq, max_seq + 1) ]

total_received = sum(received)
delivery_rate = total_received / total_sent

# 2 & 3. Longest bursts
max_success_burst = 0
max_loss_burst = 0
current_success = 0
current_loss = 0

for status in received:
    if status is True:
        current_success += 1
        max_success_burst = max(max_success_burst, current_success)
        current_loss = 0
    else:
        current_loss += 1
        max_loss_burst = max(max_loss_burst, current_loss)
        current_success = 0

print(f"Overall Delivery Rate: {delivery_rate*100:.2f}% ({total_received}/{total_sent})")
print(f"Longest consecutive success: {max_success_burst}")
print(f"Longest burst of losses: {max_loss_burst}")

# 5. Autocorrelation arrays
k_values = range(-10, 11)

prob_success_given_success = []
prob_loss_given_loss = []

for k in k_values:
    # Success given Success
    ss_count, s_total = 0, 0
    # Loss given Loss
    ll_count, l_total = 0, 0
    
    for i in range(len(received)):
        j = i + k
        if 0 <= j < len(received):
            if received[i] is True:
                s_total += 1
                if received[j] is True:
                    ss_count += 1
            else:
                l_total += 1
                if received[j] is False:
                    ll_count += 1

    p_s_given_s = (ss_count / s_total) if s_total > 0 else 0
    p_l_given_l = (ll_count / l_total) if l_total > 0 else 0
    
    prob_success_given_success.append(p_s_given_s)
    prob_loss_given_loss.append(p_l_given_l)

# Graphing
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# Graph: P(Success | Success)
ax1.bar(k_values, prob_success_given_success, color='blue')
ax1.set_title('P(Success at N+k | Success at N)')
ax1.set_xlabel('k (offset)')
ax1.set_ylabel('Probability')
ax1.axhline(y=delivery_rate, color='red', linestyle='--', label=f'Unconditional P(Success) = {delivery_rate:.2f}')
ax1.set_ylim(0, 1.1)
ax1.legend()

# Graph: P(Loss | Loss)
unconditional_loss_rate = 1.0 - delivery_rate
ax2.bar(k_values, prob_loss_given_loss, color='orange')
ax2.set_title('P(Loss at N+k | Loss at N)')
ax2.set_xlabel('k (offset)')
ax2.set_ylabel('Probability')
ax2.axhline(y=unconditional_loss_rate, color='red', linestyle='--', label=f'Unconditional P(Loss) = {unconditional_loss_rate:.2f}')
ax2.set_ylim(0, 1.1)
ax2.legend()

plt.tight_layout()
plt.savefig('autocorrelation.png', dpi=300, bbox_inches='tight')
plt.show()

import numpy as np
import matplotlib.dates as mdates

# 假设由于前面的代码，你已经有了这两个列表：
# rtts = [14.3, 15.2, 14.8, ...] # 包含所有的 RTT 值 (毫秒)
# timestamps = [datetime_obj1, datetime_obj2, ...] # 包含对应的实际时间戳

# ---------------------------------------------------------
# Question 6: Minimum RTT
# ---------------------------------------------------------
min_rtt = min(rtts)
print(f"6. The minimum RTT seen over the entire interval is: {min_rtt} ms")

# ---------------------------------------------------------
# Question 7: Maximum RTT
# ---------------------------------------------------------
max_rtt = max(rtts)
print(f"7. The maximum RTT seen over the entire interval is: {max_rtt} ms")

# ---------------------------------------------------------
# Question 8: RTT as a function of time
# ---------------------------------------------------------
plt.figure(figsize=(10, 5))
plt.plot(timestamps, rtts, marker='.', linestyle='none', markersize=2, color='b')
plt.title("8. RTT Over Time")
plt.xlabel("Actual Time of Day")
plt.ylabel("RTT (ms)")

# 格式化 x 轴的时间显示
ax = plt.gca()
ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
plt.gcf().autofmt_xdate() # 自动旋转日期标签防止重叠
plt.grid(True)
plt.savefig("q8_rtt_over_time.png")
plt.show()

# ---------------------------------------------------------
# Question 9: Cumulative Distribution Function (CDF) of RTTs
# ---------------------------------------------------------
# 对 RTT 进行排序
sorted_rtts = np.sort(rtts)
# 计算对应的累积概率 (从 0 到 1)
p = np.arange(1, len(sorted_rtts) + 1) / len(sorted_rtts)

plt.figure(figsize=(8, 5))
plt.plot(sorted_rtts, p, marker='.', linestyle='-', markersize=2, color='r')
plt.title("9. CDF of RTT Distribution")
plt.xlabel("RTT (ms)")
plt.ylabel("Cumulative Proportion (0 to 1)")
plt.grid(True)
plt.savefig("q9_cdf_rtt.png")
plt.show()

# 回答分布形状：通常，由于大多数 ping 请求的延迟集中在一个很小的基础数值附近，
# 偶尔会有较高的延迟，因此该 CDF 会在最小值附近急剧上升，然后向右侧拖出长尾 (Long-tail distribution)。
print("9. Rough shape of the distribution: The curve usually rises sharply at the minimum RTT and has a long tail to the right, indicating a heavily right-skewed distribution.")

# ---------------------------------------------------------
# Question 10: Scatter plot of RTT #N vs RTT #N+1
# ---------------------------------------------------------
# 构造两组数据：RTT N 和 RTT N+1
rtt_n = rtts[:-1]
rtt_n_plus_1 = rtts[1:]

plt.figure(figsize=(8, 8))
plt.scatter(rtt_n, rtt_n_plus_1, alpha=0.5, s=5, color='g')
plt.title("10. Correlation between Ping #N and Ping #N+1")
plt.xlabel("RTT of Ping #N (ms)")
plt.ylabel("RTT of Ping #N+1 (ms)")

# 保持 x 轴和 y 轴的比例一致，以便更好地观察对角线聚集情况
# limit_max = max(max(rtt_n), max(rtt_n_plus_1))
# plt.xlim(0, limit_max)
# plt.ylim(0, limit_max)
plt.grid(True)
plt.savefig("q10_rtt_correlation.png")
plt.show()

# 计算皮尔逊相关系数
correlation_matrix = np.corrcoef(rtt_n, rtt_n_plus_1)
correlation_xy = correlation_matrix[0,1]
print(f"10. Correlation coefficient between RTT #N and RTT #N+1 is: {correlation_xy:.4f}")
print("If the dots cluster around the y=x diagonal or form a dense blob at the base RTT, it indicates how past RTT predicts future RTT.")
