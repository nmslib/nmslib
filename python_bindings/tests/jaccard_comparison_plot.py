import pandas as pd
import sys
import statsmodels.api as sm
from plotnine import ggplot, geom_point, aes, stat_smooth, geom_line, scale_x_log10, \
    scale_y_log10, theme, element_text, ylim


if  __name__ == "__main__":
    df = pd.read_csv(sys.argv[1])
    df["space_nbits"] = df.space.astype(str) + "_" + df.nbits.astype(str)
    df.memory = df.memory.astype(float)
    df.time = df.time.astype(float)
    print(df.info())

    for col in ["time", "memory"]:
        funcs = []
        for space_nbits in df.space_nbits.unique():
            sub_df = df.loc[df.space_nbits == space_nbits]
            model = sm.OLS(sub_df.num_elems, sm.add_constant(sub_df[col]))
            params = model.fit().params
            func = lambda x: params.const + x * getattr(params, col)
            funcs.append(func)

        p = (ggplot(df, aes("num_elems", col, color="space_nbits"))
         + geom_point() + geom_line()
         + scale_x_log10(limits=[10000,10000000])
         + scale_y_log10(limits=[3,10000])
         + theme(axis_text_x=element_text(rotation=90, hjust=1)))
        p.save(filename=col + ".png", height=5, width=5, units='in', dpi=300)

        # p = ggplot(aes(x="num_elems", y=col, color="space_nbits"), data=df) + geom_line() + geom_point() + stat_function(fun=funcs[0])
        # p.make()

        # fig = plt.gcf()
        # ax = plt.gca()
        # plt.gca().set_xscale('log')
        # plt.gca().set_yscale('log')
        #
        # ggsave(plot=p, filename=col + ".png")
