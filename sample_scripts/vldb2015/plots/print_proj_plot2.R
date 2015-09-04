#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE)

#pdf(args[2])
#v <- read.table(args[1])
#original <- as.numeric(v[,1])
#projection <- as.numeric(v[,2])
#plot(original, projection)
#graphics.off()


library(ggplot2)

df <- read.table(args[1], header=FALSE, sep="\t")
colnames(df) <- c("original", "projection")
#colnames(df) <- c("", "")

head(df)

p <- ggplot(df, aes(x=original,y=projection)) +
     geom_point(size=2,shape=1) +
     geom_density2d(color="red") +
     theme_bw() + 
     theme(axis.title.x=element_blank(),
           axis.title.y=element_blank(),
           text = element_text(size=20))
     #stat_binhex()

outfile <- paste(substr(args[1], 0, nchar(args[1])-4), ".pdf", sep="")

#pdf(file=outfile)
pdf(args[2])
print(p)
dev.off()

