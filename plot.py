import networkx as nx
import matplotlib.pyplot as plt

# 读取网表文件并构建 DAG
def read_netlist(file_path):
    G = nx.DiGraph()  # 创建有向图
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):  # 忽略空行和注释
                continue
            nodes = line.split()  # 按空格拆分
            if len(nodes) == 2:  # 确保是 "source target" 格式
                G.add_edge(nodes[0], nodes[1])
    return G

# 读取 netlist.txt 构造 DAG
dag = read_netlist("/home/jing/project/Netlist2Hypergraph/build/graph")

# 设置节点颜色：默认浅蓝色
node_colors = ['lightblue' if node != '19113.stratixiv_lcell_comb' else 'red' for node in dag.nodes()]

# 计算布局（有向图适合层次布局）
pos = nx.spring_layout(dag, seed=42)  # 也可以尝试 nx.planar_layout 或 nx.kamada_kawai_layout
# pos = nx.kamada_kawai_layout(dag)

# 绘制 DAG
plt.figure(figsize=(30, 20))
nx.draw(dag, pos, with_labels=True, node_color=node_colors, edge_color='gray', arrows=True)

# # 显示
# plt.title("VLSI Netlist DAG")
# plt.show()

plt.savefig("/home/jing/project/Netlist2Hypergraph/build/dag.png", dpi=300)  # 保存高质量图片
print("DAG 图已保存为 dag.png")
