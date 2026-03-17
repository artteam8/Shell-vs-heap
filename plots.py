import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

#plt.rcParams['text.usetex'] = True

sns.set_style("whitegrid")
sns.set_context("paper", font_scale=1.5)

x_smooth = np.linspace(10, 10000, 100)
o15_smooth = 0.6 * x_smooth ** 1.5
#o15_smooth = 4 * x_smooth * np.log(x_smooth)
o2_smooth = 3 * x_smooth ** (5/4)

ns = []
ops = []
with open("values.txt", "r") as file:
    read = list(map(float, file.read().splitlines()))
print(read)
ns = np.array([10**i for i in range(1, 5)])
ops = np.array(read)

plt.figure(figsize=(8, 6))
sns.lineplot(x=x_smooth, y=o15_smooth, color='green', label=r'$0.6n^{1.5} \in \Theta(n^{1.5})$')
#sns.lineplot(x=x_smooth, y=o15_smooth, color='green', label=r'$4n\,log(n) \in O(n\,log(n))$')
sns.lineplot(x=x_smooth, y=o2_smooth, color='blue', label=r'$3n^{\frac{5}{4}} \in \Theta(n^\frac{5}{4})$')
plt.yscale('log')
sns.scatterplot(x=ns, y=ops, label='Результаты эксперимента (средние значения)', color='red')
plt.legend(fontsize=12)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.title("Сравнение асимптотики и результатов для heap sort, log scale")
#plt.show()
plt.savefig('shell_hibbard.png', dpi=300, bbox_inches='tight')
